#!/bin/sh
##########################################################################
#            PROJECT  : MPC                                              #
#            VERSION  : 2.5.1                                            #
#            DATE     : 02/2014                                          #
#			 AUTHORS  : Sebastien Valat, Augustin Serraz, Jean-Yves Vet  #
#            LICENSE  : CeCILL-C                                         #
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

######################################################
#function to pad text with spaces according to string length
#Args   :
#   -$1 : Text to pad
#	-$2 : Final string lenght
#Result : print a padded string
printWithPadding()
{
	local text="${1}"
	local length="${2}"
	local textSize=${#text}
	local padding=$(($length-$textSize))
	local output="${text}"

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


#downloadDep()
#{
# 	if [ ! -r gmp-5.1.3.tar.bz2 ]; then 
#   wget ${MIRROR}/contrib/gmp-5.1.3.tar.bz2
# fi
# if [ ! -r mpfr-3.1.2.tar.bz2 ]; then 
#   wget ${MIRROR}/contrib/mpfr-3.1.2.tar.bz2
# fi
# if [ ! -r ppl-1.1.tar.bz2 ]; then 
#   wget ${MIRROR}/contrib/ppl-1.1.tar.bz2
# fi
# if [ ! -r cloog-parma-0.16.1.tar.gz ]; then
#   wget ${MIRROR}/contrib/cloog-parma-0.16.1.tar.gz
# fi

# if [ ! -r mpc-1.0.2.tar.gz ]; then
#    wget ${MIRROR}/contrib/mpc-1.0.2.tar.gz
# fi

# if [ ! -r MPC_2.5.0.tar.gz ]; then
#   wget ${MIRROR}/MPC_2.5.0.tar.gz
# fi
#}


getMirrorAddress()
{
	#extract parameter
	local outvar="$1"	
	local mirrorid="$2"
	local address=""

	case $mirrorid in
		1)
			local address=http://fs.paratools.com/mpc
			;;
         2)
           local address=http://static.paratools.com/mpc/tar
           ;;
         3)
           local address=ftp://fs.paratools.com/mpc
           ;;
         4)
           local address=ftp://paratools08.rrp.net/mpc
           ;;
         *)
           local address=ftp://paratools08.rrp.net/mpc
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
	local outvar="$1"	

	local list=`cat "${PROJECT_SOURCE_DIR}/config.txt" | cut -f 1 -d ';' |  sed -e "s/^#[0-9A-Za-z_-\ #]*//g" | xargs echo`
	eval "${outvar}=\"${list}\""
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
	local name=`echo "${1}" | tr '[:lower:]' '[:upper:]' | sed -e "s/-/_/g"`
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
	local outvar="$1"
	local cmd="$2"
	local prefix=''

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
	local outvar="$1"
	local package="$2"
	local prefix=''

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
	local file="${PROJECT_SOURCE_DIR}/tools/${1}.sh"

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
	local outputvar="$1"
	local name="$2"
	local arg="$3"

	value=`echo "$arg" | sed -e "s/^--${name}=//g"`
	eval "${outputvar}=\"${value}\""
}

######################################################
#Find the current architecture value
#Args :
# -$1 : Output arch name (target or host)
findCurrentArch ()
{
	local outputvar="$1"
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
# -$3 : User value (--with-XXX=blablable)
extractArchValue()
{
	#extract in local vars
	local outputvar="$1"
	local name="$2"
	local arg="$3"

	value=`echo "$arg" | sed -e "s/^--${name}=//g"`	
	case "$value" in
		i686)
			value="i686"
		;;
		x86_64)
			value="x86_64"
		;;
		mic|k1om)
			value="k1om"
		;;
		arm)
			value="arm"
		;;
	esac
	eval "${outputvar}=\"${value}\""
}

######################################################
#Extract parameter value and put in given variable
#Args :
# -$1 : Output variable name
# -$2 : parameter name (without -, eg. -XXX)
# -$3 : User value (-XXX=blablable)
extractParamValueAlt()
{
	#extract in local vars
	local outputvar="$1"
	local name="$2"
	local arg="$3"

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
	local outputvar="$1"
	local name="$2"
	local arg="$3"
	
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
	local prefix="$1"

	#do exports
	PATH="${prefix}/bin:${PATH}"
	LD_LIBRARY_PATH="${prefix}/lib:${LD_LIBRARY_PATH}"
	PKG_CONFIG_PATH="${prefix}/lib/pkg-config/:${prefix}/share/pkg-config/:${PKG_CONFIG_PATH}"
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
	local file="$1"

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
	local package="$1"
	local host="$2"
	local target="$3"
	local compiler="$4"
	local type="$5"
	
	local configForCompiler=`cat "${PROJECT_PACKAGE_DIR}/${package}/config.txt" | grep "all.*all.*${compiler}"`
	local config=`cat "${PROJECT_PACKAGE_DIR}/${package}/config.txt" | grep "${host} *; *${target} *; *${compiler} *; *${type}"`
	#echo "config = $config" 1>&2
	#check if config variable is not empty
	if [ ! -z "$config" ]; then	
		local supported=`echo $config | cut -f 4 -d ';' | xargs echo`
		if [ "${supported}" = "no" ]; then
			echo "Compilation not supported for ${package} with HOST=${host}, TARGET=${target} and COMPILER=${compiler}" 1>&2
		fi
	fi
    
	local options=`echo ${configForCompiler} | cut -f 6 -d ';'`
	options="${options} `echo ${config} | cut -f 6 -d ';'`"

	case ${compiler} in
		gcc)
			options="${options} CC=gcc CXX=g++ F77=gfortran"
		;;
		icc)
			options="${options} CC=icc CXX=icpc F77=ifort"
		;;
	esac

	#echo "$package: '$options', on $type" 1>&2
	#echo "------------------------------------" 1>&2
	echo $options
}

######################################################
registerPackage()
{
	if [ "${MULTI_ARCH}" = 'false' ];
	then
		ALL_PACKAGES="${ALL_PACKAGES} ${1}"
	else
		case "${3}" in
			host)
				echo "Add ALL_PACKAGES_HOST=${1}" 1>&2
				ALL_PACKAGES_HOST="${ALL_PACKAGES_HOST} ${1}"
			;;
			target)
				echo "Add ALL_PACKAGES_TARGET=${1}" 1>&2
				ALL_PACKAGES_TARGET="${ALL_PACKAGES_TARGET} ${1}"
			;;
		esac
	fi
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
	local name="$1"
	local host="$2"
	local target="$3"	
	local compiler="$4"
	local varprefix="$5"
	local template="${PROJECT_TEMPLATE_DIR}/Makefile.${6}.in"
	local type="$7"
	
	#if template is empty, use default
	if [ -z "$template" ]; then
		template="${PROJECT_TEMPLATE_DIR}/Makefile.${name}.in"
	fi
	
	#extract prefix target
	eval "prefix=\${${varprefix}_PREFIX}"
	
	#Load package options
	local version=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 2 -d ';' | xargs echo`
	local status=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 4 -d ';' | xargs echo`
	local deps=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 5 -d ';' | xargs echo`
	local run_on=`cat "${PROJECT_SOURCE_DIR}/config.txt" | grep "^${name} " | cut -f 6 -d ';' | xargs echo`
	local options=`getPackageCompilationOptions "${name}" "${host}" "${target}" "${compiler}" "${type}"`

	#extract user options
	eval "userOptions=\"\$${varprefix}_BUILD_PARAMETERS\""
	
	#add dep if want to gen help
	if [ "$GEN_DEP_HELPS" = 'true' ]; then deps="${deps} ${name}-gen-help"; fi

	#check if need to do it
	if [ "$prefix" = "${INTERNAL_KEY}" ]; then
		PACKAGE_DEPS="$deps"
		PACKAGE_VAR_NAME="$varprefix"
		PACKAGE_OPTIONS="$options"
		PACKAGE_NAME="${name}"
		PACKAGE_VERSION="${version}"
		USER_PARAMETERS="$userOptions"
		applyOnTemplate "${template}"
	else
		echo "${name}:"
	fi

	#Line to split rules
	echo ''
	#register to list
	### TODO: change to if $host = $target > cible host, sinon target
	if [ "$status" = 'install' ]; then
			registerPackage "${name}" "${run_on}" "${type}"
	fi
}

######################################################
genAbstractMultiArchBinDir()
{
	mkdir -p "$PREFIX/generic/bin" || exit 1

	for exe in $PREFIX/host/bin/*; do
		ln -sf $PREFIX/generic/bin/arch_wrapper $PREFIX/generic/bin/`basename "$exe"`
	done
}

######################################################
showHelp()
{
	cat "${PROJECT_SOURCE_DIR}/tools/help/global.txt"
	exit 1
}

######################################################
fatal()
{
	echo "$*" 1>&2
	exit 1
}
