/*
 * FileInputUtil.h
 *
 *  Created on: Mar 15, 2016
 *      Author: max
 */

#ifndef SRC_UTIL_FILEINPUTUTIL_H_
#define SRC_UTIL_FILEINPUTUTIL_H_

#include <vector>

class Entry;

class FileInputUtil {
public:
	FileInputUtil();
	virtual ~FileInputUtil();

	// parses the file at the given location in the format 'int, int, int, ...\n...'
	static std::vector<Entry*> readEntries(std::string fileLocation, size_t bitLength);
};

#endif /* SRC_UTIL_FILEINPUTUTIL_H_ */
