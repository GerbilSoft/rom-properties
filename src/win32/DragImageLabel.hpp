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
		SIZE requiredSize(void) const
		{
			return m_requiredSize;
		}

		void setRequiredSize(const SIZE &requiredSize)
		{
			if (m_requiredSize.cx != requiredSize.cx ||
			    m_requiredSize.cy != requiredSize.cy)
			{
				m_requiredSize = requiredSize;
				updateBitmaps();
			}
		}

		void setRequiredSize(int width, int height)
		{
			if (m_requiredSize.cx != width ||
			    m_requiredSize.cy != height)
			{
				m_requiredSize.cx = width;
				m_requiredSize.cy = height;
				updateBitmaps();
			}
		}

		SIZE actualSize(void) const
		{
			return m_actualSize;
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
		 * Rescale an image to be as close to the required size as possible.
		 * @param req_sz	[in] Required size.
		 * @param sz		[in/out] Image size.
		 * @return True if nearest-neighbor scaling should be used (size was kept the same or enlarged); false if shrunken (so use interpolation).
		 */
		static bool rescaleImage(const SIZE &req_sz, SIZE &sz);

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
		bool m_useNearestNeighbor;

		// Icon sizes.
		SIZE m_requiredSize;	// Required size.
		SIZE m_actualSize;	// Actual size.

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
