#ifndef _GEN_PYTHON_H_
#define _GEN_PYTHON_H_
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>

#include "gen_generic.h"

namespace arpc {
class ArpcPython
{
	public:
		ArpcPython(const google::protobuf::FileDescriptor *pf);
		/*
		void buildHeader(const google::protobuf::FileDescriptor * pfile);
		void buildSource(const google::protobuf::FileDescriptor * pfile);
		std::string getHeaderContent() const { return this->hdr_stream.str(); }
		std::string getSourceContent() const { return this->src_stream.str(); }
		*/
		void buildScript(const google::protobuf::FileDescriptor * pfile);
		std::string getContent() const {return this->stream.str(); }
		Options opts;

	private:
		std::ostringstream unrollMethod(const google::protobuf::ServiceDescriptor * serv, bool);
		std::ostringstream unrollService(const google::protobuf::FileDescriptor * pfile);
		/*
		std::ostringstream hdr_stream;
		std::ostringstream src_stream;
		*/
		std::ostringstream stream;
		std::string filename;
};

}

#endif /* ifndef _GEN_HELPER_H_ */
