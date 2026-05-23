#include "file_tree.hpp"
#include <cassert>
#include <filesystem>

void fxed::FileTree::FileNode::print(std::ostream &os, int indent) const {
	for (int i = 0; i < indent; ++i)
		os << "  ";
	os << path.filename().string() << "\n";
}

void fxed::FileTree::DirectoryNode::print(std::ostream &os, int indent) const {
	for (int i = 0; i < indent; ++i)
		os << "  ";

	if (opened) os << "  ";
	else os << "  ";
	os << "" << path.filename().string() << "/\n";

	if (!opened) return;
	for (const auto &child : children)
		child->print(os, indent + 1);
}

void fxed::FileTree::DirectoryNode::toggleOpen() {
	opened = !opened;
	if (!updated) {
		updated = true;
		children.clear();
		for (const auto &entry : std::filesystem::directory_iterator(path)) {
			if (entry.is_directory()) {
				auto dirNode  = std::make_unique<DirectoryNode>();
				dirNode->path = entry.path();
				children.push_back(std::move(dirNode));
			} else if (entry.is_regular_file()) {
				auto fileNode  = std::make_unique<FileNode>();
				fileNode->path = entry.path();
				children.push_back(std::move(fileNode));
			}
		}
	}
}

fxed::FileTree::FileTree(const std::filesystem::path &rootPath) {
	if (!std::filesystem::is_directory(rootPath)) { throw std::invalid_argument("Root path must be a directory"); }

	root		 = std::make_unique<DirectoryNode>();
	root->path	 = rootPath;
	root->opened = true;

	for (const auto &entry : std::filesystem::directory_iterator(rootPath)) {
		if (entry.is_directory()) {
			auto dirNode  = std::make_unique<DirectoryNode>();
			dirNode->path = entry.path();
			root->children.push_back(std::move(dirNode));
		} else if (entry.is_regular_file()) {
			auto fileNode  = std::make_unique<FileNode>();
			fileNode->path = entry.path();
			root->children.push_back(std::move(fileNode));
		}
	}
}

void fxed::FileTree::print(std::ostream &os) const {
	os << root->path.string() << "/\n";
	auto *rootDir = dynamic_cast<DirectoryNode *>(root.get());
	assert(rootDir);
	if (!rootDir->opened) return;
	for (const auto &child : rootDir->children)
		child->print(os, 1);
}
