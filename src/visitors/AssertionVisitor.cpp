/*
 * AssertionVisitor.cpp
 *
 *  Created on: Mar 10, 2016
 *      Author: max
 */

#include <assert.h>
#include "AssertionVisitor.h"
#include "../NodeIterator.h"
#include "../LHC.h"
#include "../AHC.h"

AssertionVisitor::AssertionVisitor() {
}

AssertionVisitor::~AssertionVisitor() {
}

void AssertionVisitor::visit(LHC* node, unsigned int depth) {
	long suffixLength = -1;
	for (NodeIterator* it = node->begin(); (*it) <= *(node->end()); ++(*it)) {
		assert(node->prefix_.size() == node->dim_);
		NodeAddressContent content = *(*it);
		if (suffixLength < 0 && content.contained && !content.hasSubnode) {
			suffixLength = content.suffix->at(0).size();
		}

		assert(content.contained);
		assert(content.hasSubnode || content.suffix->size() == node->dim_);
		assert((content.hasSubnode || content.suffix->at(0).size() == suffixLength)
						&& "all suffixes in one node should have the same length");
	}

}

void AssertionVisitor::visit(AHC* node, unsigned int depth) {
	long suffixLength = -1;
	for (NodeIterator* it = node->begin(); (*it) <= *(node->end()); ++(*it)) {
		assert(node->prefix_.size() == node->dim_);
		NodeAddressContent content = *(*it);
		if (suffixLength < 0 && content.contained && !content.hasSubnode) {
			suffixLength = content.suffix->at(0).size();
		}

		assert(content.contained);
		assert(content.hasSubnode || content.suffix->size() == node->dim_);
		assert((content.hasSubnode || content.suffix->at(0).size() == suffixLength)
						&& "all suffixes should have the same length but was different for address" + content.address);
	}
}

void AssertionVisitor::reset() {
}