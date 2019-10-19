/***************************************************************************
 * ROM Properties Page shell extension. (Win32)                            *
 * DragImageLabel.hpp: Drag & Drop image label.                            *
 *                                                                         *
 * Copyright (c) 2019 by David Korth.                                      *
 * SPDX-License-Identifier: GPL-2.0-or-later                               *
 ***************************************************************************/

#ifndef __ROMPROPERTIES_WIN32_DRAGIMAGELABEL_HPP__
#define __ROMPROPERTIES_WIN32_DRAGIMAGELABEL_HPP__

namespace LibRpTexture {
	class rp_image;
}

// librpbase
#include "librpbase/img/IconAnimData.hpp"
#include "librpbase/img/IconAnimHelper.hpp"

// C++ includes.
#include <algorithm>
#include <array>

class DragImageLabel
{
	// TODO: Adjust image size based on DPI.
#define DIL_REQ_IMAGE_SIZE 32

	public:
		explicit DragImageLabel(HWND hwndParent);
		~DragImageLabel();

	private:
		RP_DISABLE_COPY(DragImageLabel)

	public:
		SIZE requiredImageSize(void) const
		{
			return m_requiredImageSize;
		}

		void setRequiredImageSize(const SIZE &requiredImageSize)
		{
			if (m_requiredImageSize.cx != requiredImageSize.cx ||
			    m_requiredImageSize.cy != requiredImageSize.cy)
			{
				m_requiredImageSize = requiredImageSize;
				updateBitmaps();
			}
		}

		/**
		 * Set the rp_image for this label.
		 *
		 * NOTE: The rp_image pointer is stored and used if necessary.
		 * Make sure to call this function with nullptr before deleting
		 * the rp_image object.
		 *
		 * NOTE 2: If animated icon data is specified, that supercedes
		 * the individual rp_image.
		 *
		 * @param img rp_image, or nullptr to clear.
		 * @return True on success; false on error or if clearing.
		 */
		bool setRpImage(const LibRpTexture::rp_image *img);

		/**
		 * Set the icon animation data for this label.
		 *
		 * NOTE: The iconAnimData pointer is stored and used if necessary.
		 * Make sure to call this function with nullptr before deleting
		 * the IconAnimData object.
		 *
		 * NOTE 2: If animated icon data is specified, that supercedes
		 * the individual rp_image.
		 *
		 * @param iconAnimData IconAnimData, or nullptr to clear.
		 * @return True on success; false on error or if clearing.
		 */
		bool setIconAnimData(const LibRpBase::IconAnimData *iconAnimData);

		/**
		 * Clear the rp_image and iconAnimData.
		 * This will stop the animation timer if it's running.
		 */
		void clearRp(void);

	protected:
		/**
		 * Update the bitmap(s).
		 * @return True on success; false on error.
		 */
		bool updateBitmaps(void);

	public:
		/**
		 * Start the animation timer.
		 */
		void startAnimTimer(void);

		/**
		 * Stop the animation timer.
		 */
		void stopAnimTimer(void);

		/**
		 * Is the animation timer running?
		 * @return True if running; false if not.
		 */
		bool isAnimTimerRunning(void) const
		{
			return (m_anim && m_anim->animTimerID);
		}

		/**
		 * Reset the animation frame.
		 * This does NOT update the animation frame.
		 */
		void resetAnimFrame(void)
		{
			if (m_anim) {
				m_anim->last_frame_number = 0;
			}
		}

	public:
		/**
		 * Get the current bitmap frame.
		 * @return HBITMAP.
		 */
		HBITMAP currentFrame(void) const;

	protected:
		/**
		 * Animated icon timer.
		 * @param hWnd
		 * @param uMsg
		 * @param idEvent
		 * @param dwTime
		 */
		static void CALLBACK AnimTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	private:
		HWND m_hwndParent;
		SIZE m_requiredImageSize;

		// XP theming.
		typedef BOOL (STDAPICALLTYPE* PFNISTHEMEACTIVE)(void);
		HMODULE m_hUxTheme_dll;
		PFNISTHEMEACTIVE m_pfnIsThemeActive;

		// rp_image. (NOTE: Not owned by this object.)
		const LibRpTexture::rp_image *m_img;
		HBITMAP m_hbmpImg;	// for non-animated only

		// Animated icon data.
		struct anim_vars {
			HWND m_hwndParent;
			const LibRpBase::IconAnimData *iconAnimData;
			UINT_PTR animTimerID;
			std::array<HBITMAP, LibRpBase::IconAnimData::MAX_FRAMES> iconFrames;
			LibRpBase::IconAnimHelper iconAnimHelper;
			int last_frame_number;		// Last frame number.

			anim_vars(HWND hwndParent)
				: m_hwndParent(hwndParent)
				, animTimerID(0)
				, last_frame_number(0) { }
			~anim_vars()
			{
				if (animTimerID) {
					KillTimer(m_hwndParent, animTimerID);
				}
				std::for_each(iconFrames.cbegin(), iconFrames.cend(),
					[](HBITMAP hbmp) { if (hbmp) { DeleteObject(hbmp); } }
				);
			}
		};
		anim_vars *m_anim;
};

#endif /* __ROMPROPERTIES_WIN32_DRAGIMAGELABEL_HPP__ */
