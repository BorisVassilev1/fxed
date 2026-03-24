#pragma once

#include "mesh.hpp"
#include "nri.hpp"
#include "resource_manager.hpp"
#include "text_editor.hpp"
#include "text_rendering.hpp"

namespace fxed {

class Pane {
	static ResourceID backgroundShaderID;
	static ResourceID backgroundMeshID;

	glm::ivec2 position;
	glm::ivec2 size;

   public:
	Pane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width = 800, uint32_t height = 600);
	virtual void render(nri::CommandBuffer &cmdBuf);

	static Pane *activePane;
	static Pane *getActivePane();

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	virtual void resize(uint32_t newWidth, uint32_t newHeight);
	virtual void setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height);
	virtual void scroll(int deltaX, int deltaY);
};

class TextPane : public Pane {
   protected:
	TextRenderer   &textRenderer;
	TextRenderState renderState;
	TextMesh		textMesh;

   public:
	TextPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, TextRenderer &textRenderer);
	void render(nri::CommandBuffer &cmdBuf) override;

	void scroll(int deltaX, int deltaY) override;
	void resize(uint32_t newWidth, uint32_t newHeight) override;
	void setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) override;
};

class TextEditorPane : public TextPane {
   protected:
	TextEditor editor;

   public:
	TextEditorPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, TextRenderer &textRenderer,
				   TextEditor &&editor = TextEditor());
	void render(nri::CommandBuffer &cmdBuf) override;

	TextEditor &getEditor() { return editor; }
};

}	  // namespace fxed
