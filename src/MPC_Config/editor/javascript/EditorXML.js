/*
 * XML helpers.
 */
var XML = {};

/*
 * Convert a string into XML format.
 * \fn XMLDocument StringToXML(text)
 * \param String text String to convert into XML.
 * \return XMLDocument The text converted in XML format.
 */
XML.StringToXML = function (text) {
	var parser = new DOMParser();
	doc = parser.parseFromString(text, "text/xml");
	return doc;
};

/*
 * Convert a string into XML format.
 * \fn XMLDocument StringToXML(text)
 * \param String text String to convert into XML.
 * \return XMLDocument The text converted in XML format.
 */
XML.XMLToString = function (xml) {
	var serializer = new XMLSerializer();
	string = serializer.serializeToString(xml);
	return string;
};

XML.formatXML = function(xml) {
	var formatted = '';
	var reg = /(>)(<)(\/*)/g;
	xml = xml.replace(reg, '$1\r\n$2$3');
	var splitted_xml = xml.split('\r\n');
	var pad = 0;
	
	for (var i in splitted_xml) {
		var indent = 0;
		if (splitted_xml[i].match(/.+<\/\w[^>]*>$/)) {
			indent = 0;
		}
		else if (splitted_xml[i].match(/^<\/\w/)) {
			if (pad != 0) {
				pad -= 1;
			}
		}
		else if (splitted_xml[i].match(/^<\w[^>]*[^\/]>.*$/)) {
			indent = 1;
		}
		else {
			indent = 0;
		}
		
		var padding = '';
		for (var j = 0; j < pad; j ++) {
			padding += '  ';
		}
		
		formatted += padding + splitted_xml[i] + '\r\n';
		pad += indent;
	}
	
	return formatted;
}

/*
 * Get all the nodes matching to a name in a document.
 * \fn NodeList getNodes(document, id)
 * \param XMLDocument document Document to explore.
 * \param String name Name of the tags to get.
 * \return NodeList All the nodes matching to the given name.
 */
XML.getNodes = function (document, name) {
	return document.getElementsByTagName(name);
};

/*
 * Determine if a element is a child of an other.
 * \fn Node hasChild(parent, child)
 * \param Node parent Node to examine.
 * \param String child Name of the element to look for.
 * \return Node The child node if it exists, null otherwise .
 */
XML.hasChild = function (parent, child) {
	var child_nodes = this.getChildNodes(parent);

	for (var node in child_nodes) {
		if (child_nodes[node].nodeName == child) {
			return child_nodes[node];
		}
	}

	return null;
};

/*
 * Get child nodes of a given node.
 * \fn NodeList getChildNodes(element)
 * \param Node The node to get child nodes on.
 * \return NodeList The Node.ELEMENT_NODE child nodes of the given element.
 */
XML.getChildNodes = function (element) {
	var children = [];

	if (element) {
		for (var child = element.firstChild; child; child = child.nextSibling) {
			// Only retrieve the element nodes
			if (child.nodeType == Node.ELEMENT_NODE) {
				children.push(child);
			}
		}
		return children;
	}

	return null;
};

/*
 * Get value from a node.
 * \fn String getNodeValue(element)
 * \param Node element The node to get value from.
 * \return String Value of the given node.
 */
XML.getNodeValue = function (element) {
	if (element.nodeType == Node.ATTRIBUTE_NODE) {
		return element.nodeValue;
	} else {
                if(element.childNodes[0])
                {
		        return element.childNodes[0].nodeValue;
        	}
        }

        return undefined
};

/*
 * Get all elements matching to a class name.
 * \fn Array getElementsByClassName(classname, element)
 * \param String classname Name of the searching class.
 * \param Node element If null search in the entire document, else in the element childs.
 * \return Array All the elements matching to the given class name.
 */
XML.getElementsByClassName = function (classname, element) {
	var selection = new Array();
	var regex = new RegExp("\\b" + classname + "\\b");
	
	// If no element specified, search in the entire document
	if (element == undefined) {
		element = document;
	}
	// If element matches to the id of an element
	else if (typeof element == "string") {
		element = document.getElementById(element);
	}
	
	// Get all the elements
	var elements = element.getElementsByTagName("*");
	for (var i = 0; i < elements.length; i ++) {
		if (regex.test(elements[i].className)) {
			// If the class name matches to the given one, store the element
			selection.push(elements[i]);
		}
	}
	
	return selection;
}
