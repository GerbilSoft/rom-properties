#pragma once

// Other rom-properties classes
#include "librpfile/IRpFile.hpp"

// C++ STL classes
#include <iostream>
#include <vector>

namespace LibRomData {

class streambuf_IRpFile : public std::streambuf
{
public:
	/**
	 * Create a (read-only) std::streambuf wrapper around an IRpFile.
	 * @param file IRpFilePtr
	 */
	explicit streambuf_IRpFile(const LibRpFile::IRpFilePtr &file);

protected:
	/**
	 * Read buffer has underflowed.
	 * @return Next available character
	 */
	int_type underflow() override;

	/**
	 * Seek to the specified offset in the file.
	 * @param off Offset
	 * @param way Seek from the beginning, current position, or end of the file.
	 * @param which Which pointer to adjust: in, out, or both
	 */
	pos_type seekoff(off_type off,
		std::ios_base::seekdir way,
		std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

	/**
	 * Seek to the specified position in the file.
	 * @param sp Position (from the beginning of the file)
	 * @param which Which pointer to adjust: in, out, or both
	 */
	pos_type seekpos(pos_type sp, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

private:
	static constexpr size_t BUFFER_SIZE = 4096U;
	LibRpFile::IRpFilePtr m_file;
	std::vector<char> m_in_buffer;
};

} // namespace libromdata
