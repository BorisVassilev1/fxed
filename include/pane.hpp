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

	uint32_t width;
	uint32_t height;

   public:
	Pane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width = 800, uint32_t height = 600);
	virtual void render(nri::CommandBuffer &cmdBuf);

	static Pane *activePane;
	static Pane *getActivePane();

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	virtual void resize(uint32_t newWidth, uint32_t newHeight);
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
