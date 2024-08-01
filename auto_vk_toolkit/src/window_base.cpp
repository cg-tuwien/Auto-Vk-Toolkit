#include "context_vulkan.hpp"

namespace avk
{
	uint32_t window_base::mNextWindowId = 0u;

	window_base::window_base()
		: mIsInUse(false)
		, mWindowId(mNextWindowId++)
		, mTitle()
		, mMonitor()
		, mIsInputEnabled(true)
		, mRequestedSize{ 512, 512 }
		, mCursorPosition{ 0.0, 0.0 }
		, mResolution{ 0, 0 }
		, mCursorMode{ cursor::arrow_cursor }
	{
	}

	window_base::~window_base()
	{ 
		if (mHandle) {
			context().close_window(*this);
			mHandle = std::nullopt;
		}
	}

	window_base::window_base(window_base&& other) noexcept
		: mIsInUse(std::move(other.mIsInUse))
		, mWindowId(std::move(other.mWindowId))
		, mHandle(std::move(other.mHandle))
		, mTitle(std::move(other.mTitle))
		, mMonitor(std::move(other.mMonitor))
		, mIsInputEnabled(std::move(other.mIsInputEnabled))
		, mRequestedSize(std::move(other.mRequestedSize))
		, mCursorPosition(std::move(other.mCursorPosition))
		, mResolution(std::move(other.mResolution))
		, mCursorMode(std::move(other.mCursorMode))
	{
		other.mIsInUse = false;
		other.mWindowId = 0u;
		other.mHandle = std::nullopt;
		other.mTitle = "moved from";
		other.mMonitor = std::nullopt;
		other.mIsInputEnabled = false;
		other.mCursorPosition = { 0.0, 0.0 };
		other.mResolution = { 0, 0 };
	}

	window_base& window_base::operator= (window_base&& other) noexcept
	{
		mIsInUse = std::move(other.mIsInUse);
		mWindowId = std::move(other.mWindowId);
		mHandle = std::move(other.mHandle);
		mTitle = std::move(other.mTitle);
		mMonitor = std::move(other.mMonitor);
		mIsInputEnabled = std::move(other.mIsInputEnabled);
		mRequestedSize = std::move(other.mRequestedSize);
		mCursorPosition = std::move(other.mCursorPosition);
		mResolution = std::move(other.mResolution);
		mCursorMode = std::move(other.mCursorMode);

		other.mIsInUse = false;
		other.mWindowId = 0u;
		other.mHandle = std::nullopt;
		other.mTitle = "moved from";
		other.mMonitor = std::nullopt;
		other.mIsInputEnabled = false;
		other.mCursorPosition = { 0.0, 0.0 };
		other.mResolution = { 0, 0 };

		return *this;
	}

	void window_base::initialize_after_open()
	{
		assert(context().are_we_on_the_main_thread());
		switch (glfwGetInputMode(handle()->mHandle, GLFW_CURSOR)) {
		case GLFW_CURSOR_DISABLED:
			mCursorMode = cursor::cursor_disabled_raw_input;
			break;
		case GLFW_CURSOR_HIDDEN:
			mCursorMode = cursor::cursor_hidden;
			break;
		case GLFW_CURSOR_NORMAL:
			mCursorMode = cursor::arrow_cursor;
			break;
		}
	}

	void window_base::set_is_in_use(bool value)
	{
		mIsInUse = value;
	}

	float window_base::aspect_ratio() const
	{
		auto res = resolution();
		return static_cast<float>(res.x) / static_cast<float>(res.y);
	}

	void window_base::set_resolution(window_size pExtent)
	{
		if (is_alive()) {
			context().dispatch_to_main_thread([this, pExtent]() {
				glfwSetWindowSize(handle()->mHandle, pExtent.mWidth, pExtent.mHeight);
			});
		}
		else {
			mRequestedSize = pExtent;
		}
	}

	void window_base::update_resolution()
	{
		assert(is_alive());
		context().dispatch_to_main_thread([this]() {
			int width = 0, height = 0;
			glfwGetFramebufferSize(handle()->mHandle, &width, &height);
			mResolution = glm::uvec2(width, height);
		});
	}

	void window_base::set_title(std::string _Title)
	{
		mTitle = _Title;
		if (mHandle.has_value()) {
			context().dispatch_to_main_thread([this, _Title]() {
				glfwSetWindowTitle(mHandle->mHandle, _Title.c_str());
			});
		}
	}

	void window_base::set_is_input_enabled(bool pValue)
	{
		mIsInputEnabled = pValue; 
	}

	void window_base::switch_to_fullscreen_mode(monitor_handle pOnWhichMonitor)
	{
		if (is_alive()) {
			context().dispatch_to_main_thread([this, pOnWhichMonitor]() {
				glfwSetWindowMonitor(handle()->mHandle, pOnWhichMonitor.mHandle,
									 0, 0,
									 mRequestedSize.mWidth, mRequestedSize.mHeight, // TODO: Support different resolutions or query the current resolution
									 GLFW_DONT_CARE); // TODO: Support different refresh rates
			});
		}
		else {
			mMonitor = std::nullopt;
		}
	}

	void window_base::switch_to_windowed_mode()
	{
		if (is_alive()) {
			context().dispatch_to_main_thread([this]() {
				int xpos = 10, ypos = 10;
				glfwGetWindowPos(handle()->mHandle, &xpos, &ypos);
				glfwSetWindowMonitor(handle()->mHandle, nullptr,
									 xpos, ypos,
									 mRequestedSize.mWidth, mRequestedSize.mHeight, // TODO: Support different resolutions or query the current resolution
									 GLFW_DONT_CARE); // TODO: Support different refresh rates
			});
		}
		else {
			mMonitor = std::nullopt;
		}
	}

	glm::dvec2 window_base::cursor_position() const
	{
		return mCursorPosition;
	}

	glm::uvec2 window_base::resolution() const
	{
		return mResolution;
	}

	void window_base::set_cursor_mode(cursor aCursorMode)
	{
		assert(handle());
		context().dispatch_to_main_thread([this, aCursorMode]() {
			context().activate_cursor(this, aCursorMode);
			mCursorMode = aCursorMode;
			// Also update the cursor position, because window-coordinates and raw-coordinates can be different
			glfwGetCursorPos(handle()->mHandle, &mCursorPosition.x, &mCursorPosition.y);
		});
	}

	bool window_base::is_cursor_disabled() const
	{
		return mCursorMode == cursor::cursor_disabled_raw_input;	
	}

	void window_base::set_cursor_pos(glm::dvec2 pCursorPos)
	{
		assert(handle());
		context().dispatch_to_main_thread([this, newCursorPos = pCursorPos]() {
			assert(handle());
			glfwSetCursorPos(handle()->mHandle, newCursorPos.x, newCursorPos.y);
			mCursorPosition = newCursorPos;
		});
	}

	void window_base::update_cursor_position()
	{
		assert(handle());
		context().dispatch_to_main_thread([this]() {
			assert(handle());
			glfwGetCursorPos(handle()->mHandle, &mCursorPosition[0], &mCursorPosition[1]);
		});
	}

	bool window_base::should_be_closed()
	{
		assert(handle());
		context().dispatch_to_main_thread([this]{
		    assert(handle());
			// Note: The update of the value might not happen before the next frame:
            this->mShouldClose = glfwWindowShouldClose(handle()->mHandle);
		});
		return mShouldClose;
	}
}