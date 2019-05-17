#ifndef _GEN_CPP_H_
#define _GEN_CPP_H_

#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include "gen_generic.h"

namespace arpc {
class ArpcCpp
{
	public:
		ArpcCpp(const google::protobuf::FileDescriptor *pf);
		void buildHeader(const google::protobuf::FileDescriptor * pfile);
		void buildSource(const google::protobuf::FileDescriptor * pfile);
		std::string getHeaderContent() const { return this->hdr_stream.str(); }
		std::string getSourceContent() const { return this->src_stream.str(); }
		Options opts;

	private:
		void unrollHeaderMethod(const google::protobuf::ServiceDescriptor * serv, bool);
		void unrollHeaderService(const google::protobuf::FileDescriptor * pfile);
		std::ostringstream unrollSourceMethod(const google::protobuf::ServiceDescriptor * serv, bool);
		std::ostringstream unrollSourceService(const google::protobuf::FileDescriptor * pfile);
		std::ostringstream hdr_stream;
		std::ostringstream src_stream;
		std::string filename;
};

}

#endif /* ifndef _GEN_HELPER_H_ */
