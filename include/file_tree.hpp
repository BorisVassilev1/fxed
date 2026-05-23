#pragma once

#include <filesystem>
#include <vector>
#include <memory>

namespace fxed {

class FileTree {
   public:
	class FileTreeNode {
	   public:
		std::filesystem::path path;

		virtual ~FileTreeNode()									   = default;
		virtual void print(std::ostream &os, int indent = 0) const = 0;
	};

	class FileNode : public FileTreeNode {
	   public:
		void print(std::ostream &os, int indent = 0) const override;
	};

	class DirectoryNode : public FileTreeNode {
	   public:
		std::vector<std::unique_ptr<FileTreeNode>> children;
		bool									   opened  = false;
		bool									   updated = false;

		std::size_t getChildCount() const { return opened ? children.size() : 0; }

		void print(std::ostream &os, int indent = 0) const override;

		void toggleOpen();
	};

   private:
	std::unique_ptr<DirectoryNode> root;

   public:
	FileTree(const std::filesystem::path &rootPath);

	void print(std::ostream &os) const;

	class iterator {
		std::vector<std::pair<DirectoryNode *, size_t>> stack;

	   public:
		iterator(DirectoryNode *root);

		bool operator!=(const iterator &other) const;
		bool operator==(const iterator &other) const;

		bool isBegin() const;
		bool isBack() const;

		FileTreeNode &operator*() const;

		iterator &operator++();
		iterator operator++(int);
		iterator &operator--();
		iterator operator--(int);

		void printStack(std::ostream &os) const;
	};

	iterator begin() { return iterator(root.get()); }
	iterator end() { return iterator(nullptr); }
};

}	  // namespace fxed
