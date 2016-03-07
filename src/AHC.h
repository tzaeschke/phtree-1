/*
 * AHC.h
 *
 *  Created on: Feb 25, 2016
 *      Author: max
 */

#ifndef SRC_AHC_H_
#define SRC_AHC_H_

#include <vector>
#include <iostream>
#include "Node.h"
#include "NodeIterator.h"
#include "NodeAddressContent.h"

class AHC: public Node {
	friend class AHCIterator;
public:
	AHC(size_t dim, size_t valueLength);
	AHC(Node& node);
	virtual ~AHC();
	NodeIterator* begin() override;
	NodeIterator* end() override;
	std::ostream& output(std::ostream& os, size_t depth) override;

protected:
	std::vector<bool> filled_;
	std::vector<bool> hasSubnode_;
	std::vector<Node *> subnodes_;
	// entry -> value -> bit
	std::vector<std::vector<std::vector<bool>>> suffixes_;

	virtual NodeAddressContent* lookup(long address) override;
	virtual void insertAtAddress(long hcAddress, std::vector<std::vector<bool>>* suffix) override;
	virtual void insertAtAddress(long hcAddress, Node* subnode) override;
	virtual Node* adjustSize() override;
};

#endif /* SRC_AHC_H_ */
