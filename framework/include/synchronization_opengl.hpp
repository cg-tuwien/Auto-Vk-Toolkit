#pragma once

namespace cgb
{
	/** A synchronization object which allows GPU->CPU synchronization */
	struct fence
	{
		// TODO: Call `GLsync glFenceSync(GLenum condition, GLbitfield flags)` somewhere!
	};

	/** A synchronization object which allows GPU->GPU synchronization */
	struct semaphore
	{
		// Not much to do here for OpenGL, I guess.
		// TODO: probably a barrier should be submitted somewhere.
	};

}
