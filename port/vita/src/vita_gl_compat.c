/* GPU copy fallback derived from upstream Android copy_level_by_blit.
 * This narrow path is executable in the diagnostic VPK; full renderer
 * framebuffer cache integration remains a separate milestone. */
#include "vita_runtime.h"
#include <vitaGL.h>

static int copy_texture_level(GLuint source, GLuint destination, GLint level, GLsizei width, GLsizei height)
{
	GLuint framebuffers[2]; GLint previous_read, previous_draw;
	GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST); int complete;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
	glGenFramebuffers(2, framebuffers);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffers[0]);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source, 0);
	complete = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[1]);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, destination, level);
	complete &= glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	if (complete) {
		glDisable(GL_SCISSOR_TEST);
		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	} else vita_log("FBO copy BLOCKED: incomplete source/destination");
	glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
	if (scissor) glEnable(GL_SCISSOR_TEST);
	glDeleteFramebuffers(2, framebuffers);
	return complete;
}
int vita_graphics_copy_probe(void)
{
	static const unsigned char source[16] = {
		51,102,153,255, 51,102,153,255, 51,102,153,255, 51,102,153,255
	};
	GLuint textures[2], framebuffer; unsigned char result[4] = {0};
	int copied, correct; GLenum error;
	glGenTextures(2, textures);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);
	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	copied = copy_texture_level(textures[0], textures[1], 0, 2, 2);
	glGenFramebuffers(1, &framebuffer); glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);
	if (copied && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
		glFinish(); glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, result);
	}
	error = glGetError();
	correct = copied && error == GL_NO_ERROR && result[0] == 51 && result[1] == 102 && result[2] == 153 && result[3] == 255;
	vita_log("GPU framebuffer/blit copy probe: %s RGBA=%u,%u,%u,%u error=0x%x",
		correct ? "PASS" : "FAIL", result[0], result[1], result[2], result[3], error);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); glDeleteFramebuffers(1, &framebuffer);
	glBindTexture(GL_TEXTURE_2D, 0); glDeleteTextures(2, textures);
	return correct;
}
