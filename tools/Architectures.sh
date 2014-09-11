#!/bin/sh
##########################################################################
#            PROJECT  : MPC                                              #
#            VERSION  : 2.5.1                                            #
#            DATE     : 02/2014                                          #
#			 AUTHORS  : Sebastien Valat, Augustin Serraz, Jean-Yves Vet  #
#            LICENSE  : CeCILL-C                                         #
##########################################################################

#function to get the type of a Package
#Args   :
#   -$1 : Variable in witch to put the result
#   -$2 : Package name
#Result : Create var
getAliasList()
{
	#extract parameter
	outvar="${1}"	
	arch="${2}"

	aliasList=`cat "${PROJECT_SOURCE_DIR}/.config.arch.txt" | grep "^${arch} " | cut -f 2 -d ';' | xargs echo`
	aliasList="${arch} ${aliasList}"

	eval "${outvar}=\"${aliasList}\""
}

#function to get a list of architectures supported
#Args   :
#   -$1 : Variable in witch to put the result
#Result : Create var
getArchList()
{
	#extract parameter
	outvar="${1}"	

	archList=`cat "${PROJECT_SOURCE_DIR}/.config.arch.txt" | cut -f 1 -d ';' |  sed -e "s/^#[0-9A-Za-z_\-\ #]*//g" | xargs echo`
	eval "${outvar}=\"${archList}\""
}

getArch()
{
        #extract parameter
        A_out="${1}"
	checkString="${2}"

	getArchList archList
	for arch in ${archList}; do
		getAliasList aliasList "${arch}"
		for alias in ${aliasList}; do
			value=`echo "${checkString}" | grep "${alias}"`
			if [ "${value}" != "" ]; then
				eval "${A_out}=\"${arch}\""		
				return 
			fi
		done
	done
	eval "${A_out}=0"
}


getArchDirectory()
{
    #extract parameter
    A_out="${1}"
    checkString="${2}"

    getArchList archList
    for arch in ${archList}; do
        getAliasList aliasList "${arch}"
        value=`echo "${aliasList}" | grep "${checkString}"`
        if [ "${value}" != "" ]; then
            for alias in ${aliasList}; do
                if test -d "${MPC_RPREFIX}/${alias}";
                then
                    eval "${A_out}=\"${alias}\""        
                    return 
                fi
            done
        fi
    done
    eval "${A_out}=0"
}

getBinaryTarget()
{
        #extract parameter
        BT_out="${1}"
        binary="${2}"

    #check readelf exists
    ret="`command -v readelf >/dev/null 2>&1 || echo 'readelf does no exist'`"
    
    if [ "${ret}" = "" ]; then
        archDetected="`readelf -h ${binary} | grep 'Machine:' | cut -f 2 -d ':'`"
        getArch outTmp "${archDetected}"
    fi

    eval "${BT_out}=\"${outTmp}\""
}

