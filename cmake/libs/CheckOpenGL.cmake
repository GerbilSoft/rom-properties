# Check for OpenGL.

IF(ENABLE_GL)
	SET(OpenGL_GL_PREFERENCE LEGACY)
	FIND_PACKAGE(OpenGL)
	# NOTE: OPENGL_INCLUDE_DIR might not be set on Windows.
	IF (OPENGL_gl_LIBRARY OR OPENGL_INCLUDE_DIR)
		# OpenGL was found.
	ELSE()
		# OpenGL was NOT found.
		SET(ENABLE_GL OFF)
	ENDIF()
ENDIF()
