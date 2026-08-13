/*********************************************************************************
 * ROM Properties Page shell extension. (librpbase)                              *
 * T_img_vars.hpp: Templated non_anim_vars_t and anim_vars_t for DragImageLabel. *
 *                                                                               *
 * Copyright (c) 2019-2026 by David Korth.                                       *
 * SPDX-License-Identifier: GPL-2.0-or-later                                     *
 *********************************************************************************/

#pragma once

#include "IconAnimData.hpp"
#include "IconAnimHelper.hpp"

// Other rom-properties libraries
#include "librptexture/img/rp_image.hpp"

// C++ STL classes
#include <vector>

#ifdef HAVE_STD_VARIANT
#  include <variant>
#else /* !HAVE_STD_VARIANT */
// std::variant<> is not available on this system.
// Use mpark variant instead.
#  include "mpark/variant.hpp"
namespace std {
	using mpark::variant;
	using mpark::holds_alternative;
	using mpark::get;
	using mpark::monostate;
}
#endif /* HAVE_STD_VARIANT */

namespace LibRpBase {

// NOTE: Default () init works for C++ objects (default constructor), pointers (nullptr), and primitive types (0).

// Default deleter for non-pointer ImgClass and TmrClass types, e.g. QPixmap and QTimer.
template<typename klass>
class non_pointer_deleter
{
public:
	using pointer = typename std::conditional<std::is_pointer<klass>::value, klass, klass*>::type;
	using RefClass = typename std::conditional<std::is_pointer<klass>::value, klass, klass&>::type;

	void operator()(RefClass obj)
	{
		// Default deleter does nothing; klass will delete itself.
		((void)obj);
	}
};


// Non-animated icon data
template<typename ImgClass, typename ImgClassDeleter = non_pointer_deleter<ImgClass>>
struct T_non_anim_vars_t
{
	LibRpTexture::rp_image_const_ptr img;
	ImgClass imgClass;

	explicit T_non_anim_vars_t()
		: imgClass()
	{}
	explicit T_non_anim_vars_t(const LibRpTexture::rp_image_const_ptr &img)
		: img(img)
		, imgClass()
	{}
	explicit T_non_anim_vars_t(LibRpTexture::rp_image_const_ptr &&img)
		: img(img)
		, imgClass()
	{}

	~T_non_anim_vars_t()
	{
		ImgClassDeleter()(imgClass);
	}
};

// Animated icon data
template<typename ImgClass, typename TmrClass, typename ImgClassDeleter = non_pointer_deleter<ImgClass>, typename TmrClassDeleter = non_pointer_deleter<TmrClass>>
struct T_anim_vars_t
{
	IconAnimDataConstPtr iconAnimData;
	std::vector<ImgClass> iconFrames;
	IconAnimHelper iconAnimHelper;
	TmrClass tmrIconAnim;
	int last_frame_number;	// Last frame number

	// GTK only
	int last_delay;		// Last delay value

	explicit T_anim_vars_t()
		: tmrIconAnim()
		, last_frame_number(0)
		, last_delay(0)
	{}
	explicit T_anim_vars_t(const LibRpBase::IconAnimDataConstPtr &iconAnimData)
		: iconAnimData(iconAnimData)
		, tmrIconAnim()
		, last_frame_number(0)
		, last_delay(0)
	{}
	explicit T_anim_vars_t(LibRpBase::IconAnimDataConstPtr &&iconAnimData)
		: iconAnimData(iconAnimData)
		, tmrIconAnim()
		, last_frame_number(0)
		, last_delay(0)
	{}

	~T_anim_vars_t()
	{
		auto imgClassDeleter = ImgClassDeleter();
		for (auto iter = iconFrames.begin(); iter != iconFrames.end(); ++iter) {
			imgClassDeleter(*iter);
		}
		TmrClassDeleter()(tmrIconAnim);
	}

	/**
	 * Stop/delete the timer.
	 */
	void unregister_timer(void)
	{
		TmrClassDeleter()(tmrIconAnim);
		tmrIconAnim = TmrClass();
	}

	/**
	 * Get frame 0.
	 * @return Frame 0, or nullptr on error.
	 */
	ImgClass frame0(void) const
	{
		if (!iconAnimData || iconAnimData->seq_count <= 0) {
			// No animation sequence.
			// We might still have a static icon, though...
			if (iconFrames.size() >= 1) {
				return iconFrames[0];
			}
			// No icons at all.
			return {};
		}

		const int frame0_idx = iconAnimData->seq_index[0];
		if (frame0_idx < 0 || frame0_idx >= static_cast<int>(iconFrames.size())) {
			return {};
		}

		return iconFrames[frame0_idx];
	}
};

// Class for managing a variant containing both non_anim_vars_t and anim_vars_t.
template<typename ImgClass, typename TmrClass, typename ImgClassDeleter = non_pointer_deleter<ImgClass>, typename TmrClassDeleter = non_pointer_deleter<TmrClass>>
class T_img_vars_t
{
public:
	using non_anim_vars_t = T_non_anim_vars_t<ImgClass, ImgClassDeleter>;
	using anim_vars_t = T_anim_vars_t<ImgClass, TmrClass, ImgClassDeleter, TmrClassDeleter>;

	void clear(void)
	{
		m_imgData = std::monostate();
	}

	void set(const LibRpTexture::rp_image_const_ptr &img)
	{
		m_imgData.template emplace<non_anim_vars_t>(img);
	}
	void set(LibRpTexture::rp_image_const_ptr &&img)
	{
		m_imgData.template emplace<non_anim_vars_t>(img);
	}

	void set(const IconAnimDataConstPtr &iconAnimData)
	{
		m_imgData.template emplace<anim_vars_t>(iconAnimData);
	}
	void set(IconAnimDataConstPtr &&iconAnimData)
	{
		m_imgData.template emplace<anim_vars_t>(iconAnimData);
	}

	// Convenience functions to check both if the correct type is
	// set in the variant and if the shared_ptr is not nullptr.
	inline bool isAnim(void) const
	{
		if (std::holds_alternative<anim_vars_t>(m_imgData)) {
			const anim_vars_t &anim = std::get<anim_vars_t>(m_imgData);
			return static_cast<bool>(anim.iconAnimData);
		}
		return false;
	}
	inline bool isNonAnim(void) const
	{
		if (std::holds_alternative<non_anim_vars_t>(m_imgData)) {
			const non_anim_vars_t &non_anim = std::get<non_anim_vars_t>(m_imgData);
			return (non_anim.img && non_anim.img->isValid());
		}
		return false;
	}

	non_anim_vars_t &nonAnimVars(void)
	{
		return std::get<non_anim_vars_t>(m_imgData);
	}
	const non_anim_vars_t &nonAnimVars(void) const
	{
		return std::get<non_anim_vars_t>(m_imgData);
	}

	anim_vars_t &animVars(void)
	{
		return std::get<anim_vars_t>(m_imgData);
	}
	const anim_vars_t &animVars(void) const
	{
		return std::get<anim_vars_t>(m_imgData);
	}

private:
	std::variant<std::monostate, non_anim_vars_t, anim_vars_t> m_imgData;
};

}
