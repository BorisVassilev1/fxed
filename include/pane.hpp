#pragma once

#include "file_tree.hpp"
#include "mesh.hpp"
#include "nri.hpp"
#include "resource_manager.hpp"
#include "text_editor.hpp"
#include "text_rendering.hpp"
#include "utf8_convert.hpp"

#include <filesystem>
#include <vector>
#include <string>

namespace fxed {

class Pane {
   protected:
	static ResourceID  backgroundShaderID;
	static ResourceID  backgroundMeshID;
	nri::NRI		  &nri;
	nri::CommandQueue &queue;

	glm::ivec2	   position;
	glm::ivec2	   size;
	int			   borderSize = 1;
	std::u32string name;

   public:
	Pane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width = 800, uint32_t height = 600,
		 std::u32string_view name = U"Pane");
	virtual ~Pane() = default;
	DELETE_COPY_AND_ASSIGNMENT(Pane);
	virtual void render(nri::CommandBuffer &cmdBuf);

	static Pane *activePane;
	static Pane *getActivePane();

	uint32_t			getWidth() const;
	uint32_t			getHeight() const;
	void				setActive();
	bool				containsPoint(glm::ivec2 point) const;
	std::u32string_view getName() const;

	virtual void resize(uint32_t newWidth, uint32_t newHeight);
	virtual void setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height);
	virtual void scroll(fxed::Mouse &mouse, double deltaX, double deltaY);
	virtual void mouseClick(fxed::Mouse &mouse, int button, int action, int mods);
	virtual void mouseMove(fxed::Mouse &mouse, double deltaX, double deltaY);
	virtual void charInput(unsigned int codepoint);
	virtual void keyInput(int key, int scancode, int action, int mods);

	virtual void undo() {}
	virtual void redo() {}
};

class TextPane : public Pane {
   protected:
	TextRenderer   &textRenderer;
	uint32_t		textRendererVersion;
	TextRenderState renderState;
	glm::ivec2		cursorPos;
	TextMesh		textMesh;
	std::u32string	text;
	float			scrollSpeed = 2.f;

   public:
	bool wordWrap = true;

	TextPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, TextRenderer &textRenderer);
	void render(nri::CommandBuffer &cmdBuf) override;

	void scroll(fxed::Mouse &mouse, double deltaX, double deltaY) override;
	void resize(uint32_t newWidth, uint32_t newHeight) override;
	void setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) override;
	void updateText(const std::u32string &text);
	void updateText(fxed::any_input_range<char32_t> &&text);
};

class TextEditorPane : public TextPane {
   protected:
	DefaultTextEditor editor;

   public:
	TextEditorPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, TextRenderer &textRenderer,
				   DefaultTextEditor &&editor = DefaultTextEditor());

	void render(nri::CommandBuffer &cmdBuf) override;

	DefaultTextEditor &getEditor() { return editor; }

	void charInput(unsigned int codepoint) override;
	void keyInput(int key, int scancode, int action, int mods) override;

	void undo() override;
	void redo() override;
};

class FileTextEditorPane : public TextEditorPane {
   protected:
	std::filesystem::path filePath;

   public:
	FileTextEditorPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height,
					   TextRenderer &textRenderer, const std::filesystem::path &filePath);

	const std::filesystem::path &getFilePath() const;

	void saveToFile();
};

class SplitPane : public Pane {
   protected:
	std::shared_ptr<Pane> child1;
	std::shared_ptr<Pane> child2;
	bool				  isVertical;	  // true for vertical split, false for horizontal split
	float				  splitRatio;	  // between 0 and 1

	bool isDragging = false;

   public:
	SplitPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, bool isVertical = true,
			  float splitRatio = 0.5f);
	void render(nri::CommandBuffer &cmdBuf) override;

	void resize(uint32_t newWidth, uint32_t newHeight) override;
	void setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) override;
	void mouseClick(fxed::Mouse &mouse, int button, int action, int mods) override;
	void mouseMove(fxed::Mouse &mouse, double deltaX, double deltaY) override;
	void scroll(fxed::Mouse &mouse, double deltaX, double deltaY) override;

	void				   setSplitRatio(float ratio);
	void				   setVertical(bool isVertical);
	void				   setChild(std::shared_ptr<Pane> &&child, int index);
	std::shared_ptr<Pane> &getChild(int index);
};

class FileTreePane : public TextPane {
   protected:
	std::filesystem::path currentPath;
	FileTree			  fileTree;
	int					  selectedRow;
	FileTree::iterator	  selectedIt;

	void refreshListing();

   public:
	FileTreePane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, TextRenderer &textRenderer);

	void render(nri::CommandBuffer &cmdBuf) override;
	void mouseClick(fxed::Mouse &mouse, int button, int action, int mods) override;

	void keyInput(int key, int scancode, int action, int mods) override;
	void setPath(const std::filesystem::path &p);

	const std::filesystem::path &getPath() const;
};

class TabsPane : public Pane {
   protected:
	std::vector<std::shared_ptr<Pane>> tabs;
	std::vector<TextMesh>			   tabMeshes;	  // TODO: optimize by using a single mesh for all tabs
	uint32_t						   activeTab = 0;
	TextRenderer					  &textRenderer;
	uint32_t						   textRendererVersion;

	void placeTab(std::shared_ptr<Pane> &pane);

   public:
	TabsPane(nri::NRI &nri, nri::CommandQueue &queue, uint32_t width, uint32_t height, TextRenderer &textRenderer);
	uint32_t addTab(std::shared_ptr<Pane> &&pane);
	void	 render(nri::CommandBuffer &cmdBuf) override;

	void mouseClick(fxed::Mouse &mouse, int button, int action, int mods) override;

	void resize(uint32_t newWidth, uint32_t newHeight) override;
	void setTransform(uint32_t posX, uint32_t posY, uint32_t width, uint32_t height) override;
	void scroll(fxed::Mouse &mouse, double deltaX, double deltaY) override;
	void mouseMove(fxed::Mouse &mouse, double deltaX, double deltaY) override;

	void								setActiveTab(uint32_t index);
	std::vector<std::shared_ptr<Pane>> &getTabs() { return tabs; }
};

}	  // namespace fxed
