#!/bin/sh
##########################################################################
#            PROJECT  : MPC                                              #
#			 AUTHORS  : Sebastien Valat, Augustin Serraz, Jean-Yves Vet  #
##########################################################################

######################################################
#Define some constants
INTERNAL_KEY='internal'
PREFIX='/usr/local'
MAKE_J=''
ALL_INTERNALS='false'
ENABLE_COLOR='false'
GEN_DEP_HELPS='false'
SUBPREFIX=''
CHECKSUM_TOOL="sha1sum"

######################################################
#includes
. "${PROJECT_SOURCE_DIR}/tools/Architectures.sh"

######################################################
# Escape var for to get rid of spaces in names
# $1 : prefix to escape
escapeName()
{
	varname="${1}"
	outputvar="`echo ${!varname} | sed -e 's/\ /\\\ /g'`"
	echo "${varname}=${outputvar}" 1>&2
	eval "${varname}=\"${outputvar}\""
}

######################################################
# Check dependencies for MPC installation
# $1 : list of packages which need dependencies
check_dependencies()
{
	list_missing_deps=""
	list_deps=""
	stop="0"
	# list of packages which need dependencies
	for package in ${1}
	do
		# fetch list of dependencies for the package
		list_deps=`cat "${PROJECT_SOURCE_DIR}/.dep_config" | grep "${package}" | cut -f 2 -d " "`
		for list_pattern in ${list_deps}
		do
			pattern=`echo "${list_pattern}" | cut -f 1 -d ':'`
			install=`echo "${list_pattern}" | cut -f 2 -d ':'`
			res="`cat deps_result.txt | grep \"${pattern}\"`"
			# if dependencies not found
			if test "`echo \"${res}\" | grep \"no\"`"; 
			then
				tmp=`echo "${list_missing_deps}" | grep "${install}"`
				# avoid doubles
				if test "$tmp" = ""; then
					# add dependencies to the list
					list_missing_deps="$list_missing_deps ${install}"
					stop="1"
				fi
			fi
		done
	done
	#stop install
	if test "${stop}" = "1";
	then
		echo "#################################################################################"
		echo "# The following packages are missing: "
		echo "# ${list_missing_deps}"
		echo "#"
		echo "# Please contact your administrator to install them or type the following"
		echo "# command if you are using a debian based distribution and you get admin rights"
		echo "# sudo apt-get install $list_missing_deps"
		echo "#################################################################################"
		if test -f "deps_result.txt"; then 
			rm deps_result.txt
		fi
		exit
	fi
}

######################################################
# Check if the install is already there
checkIfInstallAlreadyExists()
{
	host="${MPC_HOST}"
	target="${MPC_TARGET}"
	installs_file="${PREFIX}/.installs.cfg"
	
	if test ! -f "${installs_file}" ; then
		touch "${installs_file}"
	fi
	
	#Check the list of installs
	is_there="`cat ${installs_file} | grep \"${host}:${target}\"`"
	if test "${is_there}" = "${host}:${target}" ; 
	then
		#patch version already there
		echo "#################################################################################"
		echo "# Install for host:${host} and target:${target} already exists on that prefix "      
		echo "# Use --disable-check-install for bypassing this install "    	      
		echo "#################################################################################"
		exit 1
	fi
}

######################################################
# Put new install
putNewInstall()
{
	host="${MPC_HOST}"
	target="${MPC_TARGET}"
	installs_file="${PREFIX}/.installs.cfg"
	
	if test ! -f "${installs_file}" ; then
		touch "${installs_file}"
	fi
	
	#add host and target for this install to the list of installs
	is_there="`cat ${installs_file} | grep \"${host}:${target}\"`"
	if test "${is_there}" = "" ; 
	then
		#first patch version
		echo "${host}:${target}" >> "${installs_file}"
	fi
}

putNewCompiler()
{
	echo "$1::$2"
}

testCompilers()
{

	# C files
cat<<EOF > mpc_main.c
int i;
int main(int argc, char ** argv)
{
	return 0;
}
EOF

# CXX files
cat<<EOF > mpc_main.c++
int i;
int main(int argc, char ** argv)
{
	return 0;
}
EOF

# F77 files
cat <<EOF > mpc_main.fortran
      subroutine mpc_user_main
      integer ierr
      end
EOF

	list_languages="c c++ fortran"
	for language in ${list_languages}
	do
		lang_file=${COMPILER_FILE_DIRECTORY}/.${language}_compilers.cfg
		main_file=mpc_main.${language}
		for line in `cat ${lang_file}`
		do
			family="`echo ${line} | cut -d":" -f1`"
			compiler="`echo ${line} | cut -d":" -f3`"

			#switch depending on family
			if [ "${family}" = "INTEL" ];
			then
				PRIV_FLAG="-mSYMTAB_mpc_privatize"
			else
				PRIV_FLAG="-fmpc-privatize"
			fi
			${compiler} ${PRIV_FLAG} -c ${main_file} > /dev/null 2>&1
			if [ $? -eq 0 ];
			then
				pattern="Y"	
			else
				pattern="N"
			fi
			# for each line, replace first occurence of "::" by :Y: or :N:
			# parsed_line is used to match corresponding line in current file
			parsed_line="`echo $line | sed -e "s@/@\\\\\/@g"`"
			sed -i "/^${parsed_line}$/{s/::/:${pattern}:/1}" ${lang_file}
		done
	done

	rm mpc_main.c mpc_main.c++ mpc_main.fortran mpc_main.o > /dev/null 2>&1
}

######################################################
# set Compiler list and config file

createCompilerManager()
{
	COMPILER_FILE_DIRECTORY=${MPC_RPREFIX}/
	export COMPILER_FILE_DIRECTORY

	which ${CHECKSUM_TOOL} > /dev/null 2>&1
	if test "$?" != "0";
	then
		printf "Warning: This installation cannot find to ${CHECKSUM_TOOL}.\n"
		printf "Warning: Please ensure a valid checksum tool is available to fully exploit dynamic compiler swith facilities\n"
		return
	fi
	if test ! -w $HOME;
	then
		printf "Warning: HOME directory is not writable. Any modifications to compilers will directly impact the installation directory\n"
		return
	fi

	CURRENT_INSTALL_HASH="`echo "$MPC_RPREFIX" | sed -e "s#//*#/#g" | sha1sum | cut -f1 -d" "`"
	COMPILER_FILE_DIRECTORY=$HOME/.mpcompil/${CURRENT_INSTALL_HASH}
	
	mkdir -p $COMPILER_FILE_DIRECTORY
}

setCompilerList()
{
	createCompilerManager

	outputvar="$1"
	compiler="${MPC_COMPILER}"
	gcc_version=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^gcc " | cut -f 2 -d ';' | sed -e 's/\.//g' | xargs echo`
	config_file_c="${COMPILER_FILE_DIRECTORY}/.c_compilers.cfg"
	config_file_cplus="${COMPILER_FILE_DIRECTORY}/.c++_compilers.cfg"
	config_file_fort="${COMPILER_FILE_DIRECTORY}/.fortran_compilers.cfg"
	
	cat < /dev/null > "${config_file_c}"
	cat < /dev/null > "${config_file_cplus}"
	cat < /dev/null > "${config_file_fort}"

	#Adding GCC if internal is used
	if [ "${GCC_PREFIX}" = 'internal' ];
	then
		is_there="`cat ${config_file_c} | grep \"mpc-gcc_${gcc_version}\"`"
		if test "${is_there}" = "" ; 
		then
			#first patch version
			echo "GNU:Y:${MPC_RPREFIX}/`uname -m`/`uname -m`/bin/mpc-gcc_${gcc_version}" >> "${config_file_c}"
			echo "GNU:Y:${MPC_RPREFIX}/`uname -m`/`uname -m`/bin/mpc-g++_${gcc_version}" >> "${config_file_cplus}"
			echo "GNU:Y:${MPC_RPREFIX}/`uname -m`/`uname -m`/bin/mpc-gfortran_${gcc_version}" >> "${config_file_fort}"
		fi

	#adding GCC if external path is provided
	elif [ "${GCC_PREFIX}" != 'disabled' ];
	then
			putNewCompiler "GNU" "${GCC_PREFIX}/bin/gcc" >> "${config_file_c}"
			putNewCompiler "GNU" "${GCC_PREFIX}/bin/g++" >> "${config_file_cplus}"
			putNewCompiler "GNU" "${GCC_PREFIX}/bin/gfortran" >> "${config_file_fort}"
	fi
	
	#add all intel compilers found in environment
	list_of_icc=`which -a icc 2> /dev/null`
	list_of_icpc=`which -a icpc 2> /dev/null`
	list_of_ifort=`which -a ifort 2> /dev/null`
	for icc in ${list_of_icc}
	do
		is_there="`cat ${config_file_c} | grep \"${icc}$\"`"
		if test "${is_there}" = "" ; then
			putNewCompiler "INTEL" "${icc}" >> "${config_file_c}"
		fi
	done
	
	for icpc in ${list_of_icpc}
	do
		is_there="`cat ${config_file_cplus} | grep \"${icpc}$\"`"
		if test "${is_there}" = "" ; then
			putNewCompiler "INTEL" "${icpc}" >> "${config_file_cplus}"
		fi
	done
	for ifort in ${list_of_ifort}
	do
		is_there="`cat ${config_file_fort} | grep \"${ifort}$\"`"
		if test "${is_there}" = "" ; then
			putNewCompiler "INTEL" "${ifort}" >> "${config_file_fort}"
		fi
	done
	
	#add all GNU compilers found in environment
	list_of_gcc=`which -a gcc 2> /dev/null`
	for gcc in ${list_of_gcc}
	do
		install_base=`dirname $gcc`
		is_there="`cat ${config_file_c} | grep \"^${install_base}\"`"
		if test "${is_there}" = "" ; then
			putNewCompiler "GNU" "${install_base}/gcc" >> "${config_file_c}"
			putNewCompiler "GNU" "${install_base}/g++" >> "${config_file_cplus}"
			putNewCompiler "GNU" "${install_base}/gfortran" >> "${config_file_fort}"
		fi
	done

	testCompilers
}

######################################################
#function to pad text with spaces according to string length
#Args   :
#   -$1 : Text to pad
#	-$2 : Final string lenght
#Result : print a padded string
printWithPadding()
{
	text="${1}"
	length="${2}"
	textSize=${#text}
	padding=$(($length-$textSize))
	output="${text}"

	for i in `seq 1 1 ${padding}`; do
		output="${output} "
	done

	echo "$output"
}


######################################################
#function designed to print a summary of path selections and dep types
#Result : print summary
printSummary()
{
	echo "=================== SUMMARY =================="
	echo "HOST: ${MPC_HOST} | TARGET: ${MPC_TARGET} | COMPILER: ${MPC_COMPILER}"
	echo "BUILD        : ${PROJECT_BUILD_DIR}"
	echo "SOURCE       : ${PROJECT_SOURCE_DIR}"
	echo "INSTALL      : ${PREFIX}"
	getModules "listDeps"

	for dep in $listDeps; do
		prefixName=`echo "${dep}" | tr '[:lower:]' '[:upper:]' | sed -e "s/-/_/g"`
		prefixContent=`eval echo "$"${prefixName}_PREFIX`
		echo "`printWithPadding "${dep}" 13`: ${prefixContent}"
	done

	echo "=============================================="
}

######################################################
#function to detect if a package is present
#Args   :
#   -$1 : Package name
#	-$2 : Package version
#Result : Detect if an extern package is present. If not present, 
#         download the package (with the option --download-missing-deps)
findPackage()
{
	# TODO Select highest version

	package="${1}"
	getPackageVersion "packageVersion" "${package}"
	getPackageType "packageType" "${package}"

	# if not part of MPC, check if package is present
	if [ "${packageType}" = "extern" ]
	then
		if ls ${PROJECT_PACKAGE_DIR}/${package}/${package}-${packageVersion}.*.* > /dev/null 2>&1
		then
		    echo "Found." > /dev/null 2>&1
		else
		    echo "Package ${package}, version ${packageVersion} not found."
		    if [ "${DOWNLOAD}" = "enabled" ]
		    then
		    	downloadDep ${package} ${packageVersion}
		    else
		    	echo "Download disabled. Try to launch the script with --download-missing-deps"
		    	exit 1
		    fi
		fi
	fi
}

######################################################
#function to download a package
#Args   :
#   -$1 : Package name
#	-$2 : Package version
#Result : print a padded string
downloadDep()
{
	package="${1}"
	version="${2}"

	getMirrorAddress "mirrorAddress" ${MIRROR}

	echo "Downloading ${package}-${version}..."

	# Try to download archive with tar.bz2 extension
	wgetOutput=$(2>&1 wget --timestamping --progress=dot:mega \
              "${mirrorAddress}/${package}-${version}.tar.bz2")

	# Make sure the download went okay.
	if [ $? -ne 0 ]
	then
		# Try another extension
		wgetOutput=$(2>&1 wget --timestamping --progress=dot:mega \
              "${mirrorAddress}/${package}-${version}.tar.gz")

		# Make sure the download went okay.
		if [ $? -ne 0 ]
		then
	        echo "Cannot download package ${package} on mirror ${MIRROR}"
	        echo 1>&2 $0: "$wgetOutput"  Exiting.
	        exit 1
		fi
	fi

	mv ${package}-${version}.tar.* ${PROJECT_PACKAGE_DIR}/${package}/
}

######################################################
#function to get mirror addresses
#Args   :
#   -$1 : Variable in witch to put the result
#	-$2 : Mirror id 
#Result : return an URL
getMirrorAddress()
{
	#extract parameter
	outvar="$1"	
	mirrorid="$2"
	address=""

	case $mirrorid in
		1)
			address=http://fs.paratools.com/mpc/contrib
			;;
         2)
           address=http://static.paratools.com/mpc/tar/contrib
           ;;
         3)
           address=ftp://fs.paratools.com/mpc/contrib
           ;;
         4)
           address=ftp://paratools08.rrp.net/mpc/contrib
           ;;
         *)
           address=ftp://paratools08.rrp.net/mpc/contrib
           ;;
	esac

	eval "${outvar}=\"${address}\""
}


######################################################
#function to get a list of modules 
#Args   :
#   -$1 : Variable in witch to put the result
#Result : Create var
getModules()
{
	#extract parameter
	outvar="$1"	

	list=`cat "${PROJECT_SOURCE_DIR}/config.txt" | cut -f 1 -d ';' |  sed -e "s/^#.*//g" | xargs echo`
	eval "${outvar}=\"${list}\""
}

#function to get the version of a Package
#Args   :
#   -$1 : Variable in witch to put the result
#   -$2 : Package name
#Result : Create var
getPackageVersion()
{
	#extract parameter
	outvar="${1}"	
	package="${2}"

	version=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${package} " | cut -f 2 -d ';' | xargs echo`

	eval "${outvar}=\"${version}\""
}

#function to get the type of a Package
#Args   :
#   -$1 : Variable in witch to put the result
#   -$2 : Package name
#Result : Create var
getPackageType()
{
	#extract parameter
	outvar="${1}"	
	package="${2}"

	pType=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${package} " | cut -f 7 -d ';' | xargs echo`

	eval "${outvar}=\"${pType}\""
}

######################################################
#function to generate the prefix variable and set it to internal
#Args   :
#Result : Set to internal
setModulesToInternal()
{
	getModules 'LIST'

	for module in $LIST; do
		genPrefix $module
	done	
}

######################################################
#function to generate the prefix variable and set it to internal
#Args   :
#   -$1 : Module name
#Result : Create var
genPrefix()
{
	name=`echo "${1}" | tr '[:lower:]' '[:upper:]' | sed -e "s/-/_/g"`
	eval "${name}_PREFIX=\"${INTERNAL_KEY}\""
}

######################################################
#function to search prefix of CMD in path
#Args   :
#   -$1 : Variable in witch to put the result
#   -$2 : Command to search
#Result : Set the result in $prefix
getPrefixOfCmd()
{
	#extract parameters
	outvar="$1"
	cmd="$2"
	prefix=''

	#if not setup, need to find it, try by xml2-config in PATH
	if which ${cmd} 1>/dev/null 2>/dev/null; then
		#extract prefix by removin bin/cmd
		prefix=`which ${cmd} | xargs dirname | xargs dirname`
	fi
	
	#setup output
	eval "${outvar}=\"${prefix}\""
}

######################################################
#function to search prefix of CMD with pkg-config
#Args   :
#   -$1 : Variable in witch to put the result
#   -$2 : Package to search
#Result : Set the result in $prefix
getPrefixWithPkgConfig()
{
	#extract parameters
	outvar="$1"
	package="$2"
	prefix=''

	#if not setup, need to find it, try by xml2-config in PATH
	if pkg-config "${package}" --exists 1>/dev/null 2>/dev/null; then
		#extract prefix by removin bin/cmd
		prefix=`pkg-config "${package}" '--variable=prefix'`
		#if prefix var is not set, try with include dir
		if [ -z "${prefix}" ]; then
			prefix=`pkg-config "${package}" '--variable=includedir' | xargs dirname`
		fi
	fi
	
	#setup output
	eval "${outvar}=\"${prefix}\""
}

######################################################
include()
{
	#setup local var
	file="${PROJECT_SOURCE_DIR}/tools/${1}.sh"

	#check before include
	if [ -f "${file}" ]; then
		source "${PROJECT_SOURCE_DIR}/tools/${1}.sh"
	else
		echo "Missing file to include : ${file}" 1>&2
		exit 1
	fi
}

######################################################
#Extract parameter value and put in given variable
#Args :
# -$1 : Output variable name
# -$2 : parameter name (without --, eg. --with-XXX => 'with-XXX')
# -$3 : User value (--with-XXX=blablable)
extractParamValue()
{
	#extract in local vars
	outputvar="$1"
	name="$2"
	arg="$3"

	value=`echo "$arg" | sed -e "s/^--${name}=//g"`
	eval "${outputvar}=\"${value}\""
}

######################################################
# Set a var to a value if not already set
#args :
# -$1 : output variable name
# -$2 : value triggering a set
# -$3 : value to set
enableParamIfNot()
{
	#extract in local vars
	outputvar="$1"
	output="$2"
	trigger="$3"
	value="$4"

	if [ "${output}" = "${trigger}" ]
	then
		eval "${outputvar}=\"${value}\""
	fi
}

######################################################
#Find the current architecture value
#Args :
# -$1 : Output arch name (target or host)
findCurrentArch ()
{
	outputvar="$1"
	value=`uname -p`
	if [ "${value}" = "unknown" ]; then
		value=`uname -m`
	fi
	eval "${outputvar}=\"${value}\""
}

######################################################
#Extract architecture value and put in given variable
#Args :
# -$1 : Output arch name (target or host)
# -$2 : target or host name (without --, eg. --with-XXX => 'with-XXX')
# -$3 : User value (--with-XXX=value)
extractArchValue()
{
	#extract in local vars
	outputvar="$1"
	name="$2"
	arg="$3"

	EAV_value=`echo "$arg" | sed -e "s/^--${name}=//g"`	

	getArch "outTmp" ${EAV_value} 

	#check if architecture is supported
	if [ "${outTmp}" = "0" ]; then
		echo "Architecture ${EAV_value} not supported... abort compilation."
		exit 1
	fi

	eval "${outputvar}=\"${outTmp}\""
}

######################################################
#Extract parameter value and put in given variable
#Args :
# -$1 : Output variable name
# -$2 : parameter name (without -, eg. -XXX)
# -$3 : User value (-XXX=value)
extractParamValueAlt()
{
	#extract in local vars
	outputvar="$1"
	name="$2"
	arg="$3"

	value=`echo "$arg" | sed -e "s/^-${name}//g"`
	eval "${outputvar}=\"${value}\""
}

######################################################
#Extract parameter value and put in given variable
#Args :
# -$1 : Output variable name
# -$2 : parameter prefix (eg --libxml2-XXXX to add --XXX to libxml2 params)
# -$3 : User value (--libxml2-XXXX)
extractAndAddPackageOption()
{
	#extract in local vars
	outputvar="$1"
	name="$2"
	arg="$3"
	
	value=`echo "$arg" | sed -e "s/^--${name}//g"`
	eval "${outputvar}=\"\$${outputvar} ${value}\""
}

######################################################
#Enable use of prefix in env variables
# Args : 
#  -$1 : Prefix to use
enablePrefixEnv()
{
	#extract in local vars
	prefix="$1"

	#do exports
	PATH="${prefix}/bin:${PATH}"
	export PATH
	LD_LIBRARY_PATH="${prefix}/lib:${LD_LIBRARY_PATH}"
	export LD_LIBRARY_PATH
	PKG_CONFIG_PATH="${prefix}/lib/pkg-config/:${prefix}/share/pkg-config/:${PKG_CONFIG_PATH}"
	export PKG_CONFIG_PATH
}

######################################################
#Apply on template
# Args :
# - $1 : Template file to use
applyOnTemplate()
{
	if [ "${SUBPREFIX}" = "${SUBPREFIX_HOST}" ]; then
		SUBPREFIX_HOST=""
	fi
	#extract args
	file="$1"
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}
	export LD_LIBRARY_PATH

	#Check if the file exist
	if [ ! -f "$file" ]; then
		echo "Invalid file to use as template : $file" 1>&2
		exit 1
	fi

	#Search list of vars to replace
	varList=`egrep -o "@[0-9A-Za-z_-]*@" "${file}" | sort -u | sed -e 's/^@//' '-e s/@$//'`

	#Generate sed remplacement string
	sedOpts=''
	for var in $varList; do
		eval "value=\${${var}}"
		sedOpts="${sedOpts} -e 's#@${var}@#${value}#g'"
	done
	
	#apply ??? why didn't work without evel ???
	eval "sed ${sedOpts} \"${file}\"" || exit 1
}

######################################################
#Get compilation options according to HOST TARGET and COMPILER parameters 
#abort if configuration not supported
# Args :
#  -$1 : package name
#  -$2 : host
#  -$3 : target
#  -$4 : compiler
getPackageCompilationOptions()
{
	#made local
	package="$1"
	host="$2"
	target="$3"
	compiler="$4"
	type="$5"
	
	all_options=`cat "${PROJECT_PACKAGE_DIR}/${package}/config.txt" | grep "all *; *all *; *all" | cut -f 6 -d ';'`
	configForCompiler=`cat "${PROJECT_PACKAGE_DIR}/${package}/config.txt" | grep "all *; *all *; *${compiler}"`
	config=`cat "${PROJECT_PACKAGE_DIR}/${package}/config.txt" | grep "${host} *; *${target} *; *${compiler} *; *${type}"`
	#check if config variable is not empty
	if [ ! -z "$config" ]; then	
		supported=`echo $config | cut -f 4 -d ';' | xargs echo`
		if [ "${supported}" = "no" ]; then
			echo "Compilation not supported for ${package} with HOST=${host}, TARGET=${target} and COMPILER=${compiler}" 1>&2
		fi
	fi
    
	options=`echo ${configForCompiler} | cut -f 6 -d ';'`
	options="${all_options} ${options} `echo ${config} | cut -f 6 -d ';'`"
	
	
	# Make sure the compiler was not manually set (useful for ROMIO)
	# before setting it to ICC by force
	COMPILER_OVERIDE_FLAG=0
	

	echo "${options}" | grep "CC=" 2>&1 > /dev/null
	
	if test ${?} -eq 0; then
		COMPILER_OVERIDE_FLAG=1
	fi	
	
	echo "${options}" | grep "FC=" 2>&1 > /dev/null
	
	if test ${?} -eq 0; then
		COMPILER_OVERIDE_FLAG=1
	fi
	
	echo "${options}" | grep "F77=" 2>&1 > /dev/null
	
	if test ${?} -eq 0; then
		COMPILER_OVERIDE_FLAG=1
	fi
	
	if test ${COMPILER_OVERIDE_FLAG} -eq 0; then
		
		case ${compiler} in
			icc)
				options="${options} CC=icc CXX=icpc F77=ifort"
			;;
		esac
	fi

	echo $options
}	

######################################################
# Push packages to install for makefiles
# Args :
#  -$1 : package name
#  -$2 : package type
registerPackage()
{
		case "${2}" in
			host)
				ALL_PACKAGES_HOST="${ALL_PACKAGES_HOST} ${1}"
			;;
			target)
				ALL_PACKAGES_TARGET="${ALL_PACKAGES_TARGET} ${1}"
			;;
			all)
				if test "`echo \"${ALL_PACKAGES_HOST} \" | grep \"${1}\"`" = ""; then
					ALL_PACKAGES_HOST="${ALL_PACKAGES_HOST} ${1}"
				fi
				if test "`echo \"${ALL_PACKAGES_TARGET} \" | grep \"${1}\"`" = ""; then
					ALL_PACKAGES_TARGET="${ALL_PACKAGES_TARGET} ${1}"
				fi
			;;
		esac
}

######################################################
#Generate makefile rules do build package
# Args :
#  -$1 : package name (eg. libxml2),
#  -$2 : host
#  -$3 : target 
#  -$4 : compiler
#  -$5 : variable prefix (eg LIBXML2_PREFIX and check if equal to internal one)
#  -$6 : [OPTIONAL] template file to use.
setupInstallPackage()
{
	#made local
	name="${1}"
	host="${2}"
	target="${3}"	
	compiler="${4}"
	varprefix="${5}"
	template="${PROJECT_TEMPLATE_DIR}/Makefile.${6}.in"
	type="${7}"
	
	#if template is empty, use default
	if [ -z "${6}" ]; then
		template="${PROJECT_TEMPLATE_DIR}/Makefile.${name}.in"
	fi
	
	#extract prefix target
	eval "prefix=\${${varprefix}_PREFIX}"
	
	#Load package options
	version=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 2 -d ';' | xargs echo`
	status=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 4 -d ';' | xargs echo`
	deps=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 5 -d ';' | xargs echo`
	runOn=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 6 -d ';' | xargs echo`
	options=`getPackageCompilationOptions "${name}" "${host}" "${target}" "${compiler}" "${type}"`

	#extract user options
	eval "userOptions=\"\$${varprefix}_BUILD_PARAMETERS\""
	
	#add dep if want to gen help
	if [ "${GEN_DEP_HELPS}" = 'true' ]; then deps="${deps} ${name}-gen-help"; fi

	#check if need to do it
	if [ "${prefix}" = "${INTERNAL_KEY}" ]; then
		PACKAGE_DEPS=""
		for dep in `echo ${deps}`; 
		do
			UPPER=`echo ${dep} | tr '[:lower:]' '[:upper:]'`
			eval "DEP_PREFIX=\${${UPPER}_PREFIX}"
			if test "${DEP_PREFIX}" = "internal"; then
				PACKAGE_DEPS="${PACKAGE_DEPS} `echo ${dep} | sed 's|[0-9A-Z_a-z\-]\+|.&|g'`"
			fi
		done
		PACKAGE_VAR_NAME="${varprefix}"
		PACKAGE_OPTIONS="${options}"
		PACKAGE_NAME="${name}"
		PACKAGE_VERSION="${version}"
		USER_PARAMETERS="${userOptions}"
		applyOnTemplate "${template}"
	else
		echo "${name}:"
	fi

	#Line to split rules
	echo ''

	#Register to list
	### TODO: change to if $host = $target > cible host, sinon target
	if [ "${status}" = 'install' ]; then
			registerPackage "${name}" "${runOn}"
	fi
}


#function to get MPC version
#Args   :
#   -$1 : Variable in witch to put the result
#Result : Get a version string
getVersion()
{
        #extract parameter
        outvar="$1"
	MPC_SOURCE_DIR="${PROJECT_SOURCE_DIR}/mpcframework/"
	MPC_SOURCE_DIR="${MPC_SOURCE_DIR}"
	export MPC_SOURCE_DIR
	MPC_VERSION="`${MPC_SOURCE_DIR}/MPC_Tools/mpc_print_version | sed -e 's/_[a-zA-Z0-9]*//g'`"

        eval "${outvar}=\"${MPC_VERSION}\""
}

######################################################
showVersion()
{
	getVersion "MPC_VERSION"
	echo ${MPC_VERSION}
	exit 1
}

showHelp()
{
	getVersion "MPC_VERSION"
	cat "${PROJECT_SOURCE_DIR}/tools/banner"
	cat "${PROJECT_SOURCE_DIR}/tools/help/global.txt" |  sed -e "s/@MPC_VERSION@/${MPC_VERSION}/g"
	exit 1
}

######################################################
fatal()
{
	echo "$*" 1>&2
	exit 1
}
