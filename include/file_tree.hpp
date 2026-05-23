#pragma once

#include <filesystem>
#include <vector>
#include <memory>
#include "any_range.hpp"

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
		iterator(DirectoryNode *root) {
			if (root) stack.push_back({root, 0});
		}

		bool operator!=(const iterator &other) const { return !stack.empty() || !other.stack.empty(); }
		bool operator==(const iterator &other) const { return stack.empty() && other.stack.empty(); }

		bool isBegin() const { return stack.size() == 1 && stack.back().second == 0; }
		bool isBack() const {
			if (stack.empty()) return true;
			for (const auto &[dir, index] : stack)
				if (index != dir->getChildCount() - 1) return false;
			return true;
		}

		FileTreeNode &operator*() const {
			if (stack.empty()) throw std::out_of_range("Iterator out of range");
			auto &[dir, index] = stack.back();
			if (index >= dir->getChildCount()) throw std::out_of_range("Iterator out of range");
			return *dir->children[index];
		}

		iterator &operator++() {
			if (stack.empty()) throw std::out_of_range("Iterator out of range");
			auto &[dir, index] = stack.back();
			if (index >= dir->getChildCount()) throw std::out_of_range("Iterator out of range");

			auto *node = dir->children[index].get();
			++index;

			if (auto *subdir = dynamic_cast<DirectoryNode *>(node)) { stack.push_back({subdir, 0}); }

			while (!stack.empty()) {
				auto &[currentDir, currentIndex] = stack.back();
				if (currentIndex < currentDir->getChildCount()) break;
				stack.pop_back();
			}

			return *this;
		}

		iterator operator++(int) {
			iterator temp = *this;
			++(*this);
			return temp;
		}

		iterator &operator--() {
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

		iterator operator--(int) {
			iterator temp = *this;
			--(*this);
			return temp;
		}

		void printStack(std::ostream &os) const {
			for (const auto &[dir, index] : stack) {
				os << dir->path.filename().string() << " ( " << index << " ) ->";
			}
			os << std::endl;
		}
	};

	iterator begin() { return iterator(root.get()); }
	iterator end() { return iterator(nullptr); }
};

}	  // namespace fxed
