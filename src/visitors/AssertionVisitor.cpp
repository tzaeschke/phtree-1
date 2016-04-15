/*
 * AssertionVisitor.cpp
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#include <assert.h>
#include "visitors/AssertionVisitor.h"
#include "iterators/NodeIterator.h"
#include "nodes/Node.h"
#include "nodes/LHC.h"
#include "nodes/AHC.h"

AssertionVisitor::AssertionVisitor() {
}

AssertionVisitor::~AssertionVisitor() {
}

void AssertionVisitor::visit(LHC* node, unsigned int depth) {
	validateContents(node, node->begin(), node->end());
}

void AssertionVisitor::visit(AHC* node, unsigned int depth) {
	validateContents(node, node->begin(), node->end());
}

void AssertionVisitor::validateContents(Node* node, NodeIterator* begin, NodeIterator* end) {
	bool foundSuffix = false;
		size_t suffixLength = 0;
		for (NodeIterator* it = begin; (*it) != *(end); ++(*it)) {
			NodeAddressContent content = *(*it);
			if (!foundSuffix && content.exists && !content.hasSubnode) {
				suffixLength = content.suffix->size();
				foundSuffix = true;
			}

			assert(content.exists);
			assert((content.hasSubnode || content.suffix->size() == suffixLength)
							&& "all suffixes in one node should have the same length");
		}
}

void AssertionVisitor::reset() {
}
