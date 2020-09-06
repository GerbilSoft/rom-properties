/***************************************************************************
 * ROM Properties Page shell extension. (libromdata)                       *
 * ndscrypt.cpp: Nintendo DS encryption.                                   *
 *                                                                         *
 * Copyright (c) 2020 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#include "ndscrypt.hpp"

#include "byteswap.h"
#include "nds_crc.hpp"	// TODO: Optimized versions?

// C includes.
#include <stdint.h>
#include <stdlib.h>

// C includes. (C++ namespace)
#include <cassert>
#include <cerrno>

// librpbase, librpfile, librpthreads
#include "librpbase/crypto/MD5.hpp"
#include "librpfile/FileSystem.hpp"
#include "librpfile/RpFile.hpp"
#include "librpthreads/Mutex.hpp"
using namespace LibRpFile;
using LibRpBase::MD5;
using LibRpBase::Mutex;
using LibRpBase::MutexLocker;

// C++ STL classes.
using std::string;

// Blowfish data.
// This is loaded from ~/.config/rom-properties/nds-blowfish.bin.
static const uint8_t nds_blowfish_md5[16] = {
	0xC0,0x8C,0x5A,0xFD,0x9C,0x6D,0x95,0x30,
	0x81,0x7C,0xD2,0x03,0x3E,0x38,0x64,0xD7,
};
static uint8_t nds_blowfish_data[0x1048];
static Mutex nds_mutex;

/**
 * Load and verify nds-blowfish.bin.
 * This must be present in order to use ndscrypt_secure_area().
 * TODO: Custom error codes enum.
 * @return 0 on success; negative POSIX error code, positive custom error code on error.
 */
int ndscrypt_load_nds_blowfish_bin(void)
{
	if (nds_blowfish_data[0] != 0) {
		// Blowfish data was already loaded.
		return 0;
	}

	MutexLocker mutexLocker(nds_mutex);
	// Verify again after the mutex is locked.
	if (nds_blowfish_data[0] == 0x99) {
		// Blowfish data was already loaded.
		return 0;
	}

	// Get the filename.
	string bin_filename = FileSystem::getConfigDirectory();
	if (bin_filename.empty()) {
		// Unable to get the configuration directory.
		return -ENOENT;
	}
	if (bin_filename.at(bin_filename.size()-1) != DIR_SEP_CHR) {
		bin_filename += DIR_SEP_CHR;
	}
	bin_filename += "nds-blowfish.bin";

	// Open the file.
	RpFile *const f_blowfish = new RpFile(bin_filename, RpFile::FM_OPEN_READ);
	if (!f_blowfish->isOpen()) {
		// Unable to open the file.
		int err = f_blowfish->lastError();
		if (err == 0) {
			err = EIO;
		}
		UNREF(f_blowfish);
		return -err;
	}

	// File must be the correct size.
	if (f_blowfish->size() != static_cast<off64_t>(sizeof(nds_blowfish_data))) {
		// Wrong size.
		UNREF(f_blowfish);
		return 1;
	}

	// Read the file.
	size_t size = f_blowfish->read(nds_blowfish_data, sizeof(nds_blowfish_data));
	if (size != sizeof(nds_blowfish_data)) {
		// Read error.
		nds_blowfish_data[0] = 0;
		int err = f_blowfish->lastError();
		if (err == 0) {
			err = EIO;
		}
		UNREF(f_blowfish);
		return -err;
	}

	// Verify the MD5.
	uint8_t md5[16];
	MD5::calcHash(md5, sizeof(md5), nds_blowfish_data, sizeof(nds_blowfish_data));
	if (memcmp(md5, nds_blowfish_md5, sizeof(md5)) != 0) {
		// MD5 is incorrect.
		nds_blowfish_data[0] = 0;
		return 2;
	}

	// Blowfish data has been verified.
	return 0;
}

// Encryption context.
class NDSCrypt
{
	public:
		NDSCrypt(uint32_t gamecode);

	private:
		static uint32_t lookup(uint32_t *magic, uint32_t v);

		static void encrypt(uint32_t *magic, uint32_t *arg1, uint32_t *arg2);
		static void decrypt(uint32_t *magic, uint32_t *arg1, uint32_t *arg2);

		static void encrypt(uint32_t *magic, uint64_t &cmd);
		static void decrypt(uint32_t *magic, uint64_t &cmd);

		static void update_hashtable(uint32_t* magic, uint8_t arg1[8]);
		static void init2(uint32_t *magic, uint32_t a[3]);
		void init1(void);

	public:
		void init0(void);
		int decrypt_arm9(uint8_t *data);
		int encrypt_arm9(uint8_t *data);

		inline const uint32_t *card_hash(void) const
		{
			return m_card_hash;
		}

	private:
		uint32_t m_gamecode;

		uint32_t m_card_hash[0x412];
		int m_cardheader_devicetype;		// TODO: DSi?
		uint32_t m_global3_x00, m_global3_x04;	// RTC value
		uint32_t m_global3_rand1;
		uint32_t m_global3_rand3;
		uint32_t m_arg2[3];
};

NDSCrypt::NDSCrypt(uint32_t gamecode)
	: m_gamecode(gamecode)
	, m_cardheader_devicetype(0)
	, m_global3_x00(0)
	, m_global3_x04(0)
	, m_global3_rand1(0)
	, m_global3_rand3(0)
{
	memset(m_arg2, 0, sizeof(m_arg2));
}

uint32_t NDSCrypt::lookup(uint32_t *magic, uint32_t v)
{
	uint32_t a = (v >> 24) & 0xFF;
	uint32_t b = (v >> 16) & 0xFF;
	uint32_t c = (v >> 8) & 0xFF;
	uint32_t d = (v >> 0) & 0xFF;

	a = magic[a+18+0];
	b = magic[b+18+256];
	c = magic[c+18+512];
	d = magic[d+18+768];

	return d + (c ^ (b + a));
}

void NDSCrypt::encrypt(uint32_t *magic, uint32_t *arg1, uint32_t *arg2)
{
	uint32_t a,b,c;
	a = *arg1;
	b = *arg2;
	for (int i=0; i<16; i++)
	{
		c = magic[i] ^ a;
		a = b ^ lookup(magic, c);
		b = c;
	}
	*arg2 = a ^ magic[16];
	*arg1 = b ^ magic[17];
}

void NDSCrypt::decrypt(uint32_t *magic, uint32_t *arg1, uint32_t *arg2)
{
	uint32_t a,b,c;
	a = *arg1;
	b = *arg2;
	for (int i=17; i>1; i--)
	{
		c = magic[i] ^ a;
		a = b ^ lookup(magic, c);
		b = c;
	}
	*arg1 = b ^ magic[0];
	*arg2 = a ^ magic[1];
}

void NDSCrypt::encrypt(uint32_t *magic, uint64_t &cmd)
{
	encrypt(magic, (uint32_t *)&cmd + 1, (uint32_t *)&cmd + 0);
}

void NDSCrypt::decrypt(uint32_t *magic, uint64_t &cmd)
{
	decrypt(magic, (uint32_t *)&cmd + 1, (uint32_t *)&cmd + 0);
}

void NDSCrypt::update_hashtable(uint32_t* magic, uint8_t arg1[8])
{
	for (int j=0;j<18;j++)
	{
		uint32_t r3=0;
		for (int i=0;i<4;i++)
		{
			r3 <<= 8;
			r3 |= arg1[(j*4 + i) & 7];
		}
		magic[j] ^= r3;
	}

	uint32_t tmp1 = 0;
	uint32_t tmp2 = 0;
	for (int i=0; i<18; i+=2)
	{
		encrypt(magic,&tmp1,&tmp2);
		magic[i+0] = tmp1;
		magic[i+1] = tmp2;
	}
	for (int i=0; i<0x400; i+=2)
	{
		encrypt(magic,&tmp1,&tmp2);
		magic[i+18+0] = tmp1;
		magic[i+18+1] = tmp2;
	}
}

void NDSCrypt::init2(uint32_t *magic, uint32_t a[3])
{
	encrypt(magic, a+2, a+1);
	encrypt(magic, a+1, a+0);
	update_hashtable(magic, (uint8_t*)a);
}

void NDSCrypt::init1(void)
{
	memcpy(m_card_hash, &nds_blowfish_data, 4*(1024 + 18));
	m_arg2[0] = m_gamecode;
	m_arg2[1] = m_gamecode >> 1;
	m_arg2[2] = m_gamecode << 1;
	init2(m_card_hash, m_arg2);
	init2(m_card_hash, m_arg2);
}

void NDSCrypt::init0(void)
{
	init1();
	encrypt(m_card_hash, (uint32_t*)&m_global3_x04, (uint32_t*)&m_global3_x00);
	m_global3_rand1 = m_global3_x00 ^ m_global3_x04;		// more RTC
	m_global3_rand3 = m_global3_x04 ^ 0x0380FEB2;
	encrypt(m_card_hash, (uint32_t*)&m_global3_rand3, (uint32_t*)&m_global3_rand1);
}

// ARM9 decryption check values
#define MAGIC30		0x72636E65
#define MAGIC34		0x6A624F79

/*
 * decrypt_arm9
 */
int NDSCrypt::decrypt_arm9(uint8_t *data)
{
	uint32_t *p = (uint32_t*)data;

	init1();
	decrypt(m_card_hash, p+1, p);
	m_arg2[1] <<= 1;
	m_arg2[2] >>= 1;	
	init2(m_card_hash, m_arg2);
	decrypt(m_card_hash, p+1, p);

	if (p[0] != MAGIC30 || p[1] != MAGIC34)
	{
		return -EIO;
	}

	*p++ = 0xE7FFDEFF;
	*p++ = 0xE7FFDEFF;
	uint32_t size = 0x800 - 8;
	while (size > 0)
	{
		decrypt(m_card_hash, p+1, p);
		p += 2;
		size -= 8;
	}

	return 0;
}

/*
 * encrypt_arm9
 */
int NDSCrypt::encrypt_arm9(uint8_t *data)
{
	uint32_t *p = (uint32_t*)data;
	if (p[0] != cpu_to_le32(0xE7FFDEFF) || p[1] != cpu_to_le32(0xE7FFDEFF))
	{
		return -EIO;
	}
	p += 2;

	init1();

	m_arg2[1] <<= 1;
	m_arg2[2] >>= 1;
	
	init2(m_card_hash, m_arg2);

	uint32_t size = 0x800 - 8;
	while (size > 0)
	{
		encrypt(m_card_hash, p+1, p);
		p += 2;
		size -= 8;
	}

	// place header
	p = (uint32_t*)data;
	p[0] = MAGIC30;
	p[1] = MAGIC34;
	encrypt(m_card_hash, p+1, p);
	init1();
	encrypt(m_card_hash, p+1, p);
	return 0;
}

/**
 * Encrypt the secure area and create the encryption data
 * required for official flash carts and IS-NITRO.
 * @param pRom First 32 KB of the ROM image.
 * @return 0 on success; non-zero on error.
 */
static int encryptSecureArea(uint8_t *pRom)
{
	static const unsigned int rounds_offsets = 0x1600;
	static const unsigned int sbox_offsets = 0x1C00;

	// If the ROM is already encrypted, we don't need to do anything.
	uint32_t *const pRom32 = reinterpret_cast<uint32_t*>(pRom);
	if (pRom32[0x4000/4] != 0xE7FFDEFF && pRom32[0x4004/4] != 0xE7FFDEFF) {
		// ROM is already encrypted.
		return 0;
	}

	uint32_t gamecode = le32_to_cpu(pRom32[0x0C/4]);
	NDSCrypt ndsCrypt(gamecode);
	ndsCrypt.encrypt_arm9(&pRom[0x4000]);

	// Calculate CRCs.
	uint16_t *const pRom16 = reinterpret_cast<uint16_t*>(pRom);
	// Secure Area CRC16
	pRom16[0x6C/2] = cpu_to_le16(CalcCrc16(&pRom[0x4000], 0x4000));
	// Header CRC16
	pRom16[0x15E/2] = cpu_to_le16(CalcCrc16(pRom, 0x15E));

	// Reinitialize the card hash.
	ndsCrypt.init0();
	//srand(gamecode);	// FIXME: Is this actually needed?

	// rounds table
	memcpy(&pRom[rounds_offsets], ndsCrypt.card_hash(), 4*18);

	// S-boxes
	for (unsigned int i = 0; i < 4; i++) {
		memcpy(&pRom[sbox_offsets + (4*256*i)], &ndsCrypt.card_hash()[18 + (i*256)], 4*256);
	}

	// test patterns
	memcpy(&pRom[0x3000], "\xFF\x00\xFF\x00\xAA\x55\xAA\x55", 8);
	for (unsigned int i = 0x3008; i < 0x3200; i++) pRom[i] = (uint8_t)i;
	for (unsigned int i = 0x3200; i < 0x3400; i++) pRom[i] = (uint8_t)(0xFF - i);
	memset(&pRom[0x3400], 0x00, 0x200);
	memset(&pRom[0x3600], 0xFF, 0x200);
	memset(&pRom[0x3800], 0x0F, 0x200);
	memset(&pRom[0x3A00], 0xF0, 0x200);
	memset(&pRom[0x3C00], 0x55, 0x200);
	memset(&pRom[0x3E00], 0xAA, 0x200);
	pRom[0x3FFF] = 0x00;

	// Calculate CRCs and write header.
	// Secure Area CRC16
	pRom16[0x6C/2] = cpu_to_le16(CalcCrc16(&pRom[0x4000], 0x4000));
	// Logo CRC16
	pRom16[0x15C/2] = cpu_to_le16(CalcCrc16(&pRom[0xC0], 0x9C));
	// Header CRC16
	pRom16[0x15E/2] = cpu_to_le16(CalcCrc16(pRom, 0x15E));

	return 0;
}

/**
 * Encrypt the ROM's Secure Area, if necessary.
 * @param pRom First 32 KB of the ROM image.
 * @param len Length of pRom.
 * @return 0 on success; non-zero on error.
 */
int ndscrypt_encrypt_secure_area(uint8_t *pRom, size_t len)
{
	assert(len >= 32768);
	if (len < 32768)
		return -EINVAL;

	// nds-blowfish.bin must have been loaded before calling this function.
	if (nds_blowfish_data[0] != 0x99)
		return -EIO;

	// Encrypt the Secure Area.
	return encryptSecureArea(pRom);
}

/**
 * Decrypt the secure area and remove the static data.
 * @param pRom First 32 KB of the ROM image.
 * @return 0 on success; non-zero on error.
 */
static int decryptSecureArea(uint8_t *pRom)
{
	// If the ROM is already decrypted, we don't need to do anything.
	uint32_t *const pRom32 = reinterpret_cast<uint32_t*>(pRom);
	if (pRom32[0x4000/4] == 0xE7FFDEFF && pRom32[0x4004/4] == 0xE7FFDEFF) {
		// ROM is already decrypted.
		return 0;
	}

	uint32_t gamecode = le32_to_cpu(pRom32[0x0C/4]);
	NDSCrypt ndsCrypt(gamecode);
	ndsCrypt.decrypt_arm9(&pRom[0x4000]);

	// Zero out the static data.
	memset(&pRom[0x1000], 0, 0x3000);
	return 0;
}

/**
 * Decrypt the ROM's Secure Area, if necessary.
 * @param pRom First 32 KB of the ROM image.
 * @param len Length of pRom.
 * @return 0 on success; non-zero on error.
 */
int ndscrypt_decrypt_secure_area(uint8_t *pRom, size_t len)
{
	assert(len >= 32768);
	if (len < 32768)
		return -EINVAL;

	// nds-blowfish.bin must have been loaded before calling this function.
	if (nds_blowfish_data[0] != 0x99)
		return -EIO;

	// Decrypt the Secure Area.
	return decryptSecureArea(pRom);
}
