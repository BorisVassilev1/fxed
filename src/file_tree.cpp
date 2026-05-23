#include "file_tree.hpp"
#include <cassert>
#include <filesystem>

using namespace fxed;

void FileTree::FileNode::print(std::ostream &os, int indent) const {
	for (int i = 0; i < indent; ++i)
		os << "  ";
	os << path.filename().string() << "\n";
}

void FileTree::DirectoryNode::print(std::ostream &os, int indent) const {
	for (int i = 0; i < indent; ++i)
		os << "  ";

	if (opened) os << "  ";
	else os << "  ";
	os << "" << path.filename().string() << "/\n";

	if (!opened) return;
	for (const auto &child : children)
		child->print(os, indent + 1);
}

void FileTree::DirectoryNode::toggleOpen() {
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

FileTree::FileTree(const std::filesystem::path &rootPath) {
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

void FileTree::print(std::ostream &os) const {
	os << root->path.string() << "/\n";
	auto *rootDir = dynamic_cast<DirectoryNode *>(root.get());
	assert(rootDir);
	if (!rootDir->opened) return;
	for (const auto &child : rootDir->children)
		child->print(os, 1);
}

FileTree::iterator::iterator(DirectoryNode *root) {
	if (root) stack.push_back({root, 0});
}

bool FileTree::iterator::operator!=(const iterator &other) const {
	return !stack.empty() || !other.stack.empty();
}
bool FileTree::iterator::operator==(const iterator &other) const { return stack.empty() && other.stack.empty(); }

bool FileTree::iterator::isBegin() const { return stack.size() == 1 && stack.back().second == 0; }
bool FileTree::iterator::isBack() const {
	if (stack.empty()) return true;
	for (const auto &[dir, index] : stack)
		if (index != dir->getChildCount() - 1) return false;
	return true;
}

FileTree::FileTreeNode &FileTree::iterator::operator*() const {
	if (stack.empty()) throw std::out_of_range("Iterator out of range");
	auto &[dir, index] = stack.back();
	if (index >= dir->getChildCount()) throw std::out_of_range("Iterator out of range");
	return *dir->children[index];
}

FileTree::iterator &FileTree::iterator::operator++() {
	if (stack.empty()) throw std::out_of_range("Iterator out of range");
	auto &[dir, index] = stack.back();
	if (index >= dir->getChildCount()) throw std::out_of_range("Iterator out of range");

	auto *node = dir->children[index].get();

	auto *subdir = dynamic_cast<DirectoryNode *>(node);
	if (subdir && subdir->getChildCount() > 0) {
		stack.push_back({subdir, 0});
	} else {
		++index;
		while (!stack.empty() && stack.back().second >= stack.back().first->getChildCount()) {
			stack.pop_back();
			if (!stack.empty()) ++stack.back().second;
		}
	}
	return *this;
}

FileTree::iterator FileTree::iterator::operator++(int) {
	iterator temp = *this;
	++(*this);
	return temp;
}

FileTree::iterator &FileTree::iterator::operator--() {
	if (stack.empty()) throw std::out_of_range("Iterator out of range");

	auto &[dir, index] = stack.back();
	if (index == 0) {
		stack.pop_back();
		return *this;
	}

	--index;
	auto *node = dir->children[index].get();

	while (auto *subdir = dynamic_cast<DirectoryNode *>(node)) {
		if (subdir->getChildCount() == 0) break;
		stack.push_back({subdir, subdir->getChildCount() - 1});
		node = subdir->children.back().get();
	}

	return *this;
}

FileTree::iterator FileTree::iterator::operator--(int) {
	iterator temp = *this;
	--(*this);
	return temp;
}

void FileTree::iterator::printStack(std::ostream &os) const {
	for (const auto &[dir, index] : stack) {
		os << dir->path.filename().string() << " ( " << dir->children[index]->path.filename().string() << " ) ";
	}
	os << std::endl;
}
