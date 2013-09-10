if (window.File && window.FileReader && window.FileList && window.Blob) {
	// The File APIs are fully supported.
}
else {
	alert('The File APIs are not fully supported.');
}

// This hash table will contains the profiles model
var myModel = {};

// This hash table will contain the network model
var myNetworks = {};

// This hash table will contain the mappings model
var myMappings = {};

/*!
 * Check if a given type is basic one.
 * \fn Boolean isBasicType(type)
 * \param String type Type to check.
 * \return Boolean true if it is a basic type, false otherwise.
 */
function isBasicType(type) {
	var basicTypes = ["int", "string", "bool", "funcptr", "size", "array"];
	var isBasicType = basicTypes.indexOf(type);
	
	if (isBasicType != -1)
		return true; // Is a basic type
	
	return false; // Is not a basic type
}

/*!
 * Remove spaces at the beginning and at the end of a given string.
 * \fn String trim(myString)
 * \param String myString The string to trim.
 * \return String The trimmed string.
 */
function trim(myString) {
	return myString.replace(/^\s+/g,'').replace(/\s+$/g,'');
}

function getKeys(element) {
	try {
		var keys = Object.keys(element);
		return keys;
	}
	catch (err) {
		return null;
	}
}

/*
 * Helper to clone a javascript object.
 * \fn Object clone(obj)
 * \param Object obj The object to clone.
 * \return Object The cloned object.
 */
function clone(obj) {
	// Handle the 3 simple types, and null or undefined
	if (null == obj || "object" != typeof obj) return obj;

	// Handle Date
	if (obj instanceof Date) {
		var copy = new Date();
		copy.setTime(obj.getTime());
		return copy;
	}

	// Handle Array
	if (obj instanceof Array) {
		var copy = [];
		for (var i = 0, len = obj.length; i < len; i++) {
			copy[i] = clone(obj[i]);
		}
		return copy;
	}

	// Handle Object
	if (obj instanceof Object) {
		var copy = {};
		for (var attr in obj) {
			if (obj.hasOwnProperty(attr)) copy[attr] = clone(obj[attr]);
		}
		return copy;
	}

	throw new Error("Unable to copy obj! Its type isn't supported.");
}

/*!
 * Display/hide a given element.
 * \fn void printElement(element)
 * \param element Element Element to display/hide.
 */
function printElement(element) {
	var id_element = "";
	
	// element can be either the id of the element..
	if (typeof element == "string") {
		id_element = element;
		element = document.getElementById(id_element);
	}
	// .. or the element itself
	else {
		id_element = element.id
	}
	
	// Display/hide the associated element
	var id_vars = id_element + "_vars";
	var element_vars = document.getElementById(id_vars);
	
	if (element_vars) {
		var span_title = element.getElementsByTagName('span')[0];
		var image = (element_vars.style.display == "none" ? "./images/expanded.gif" : "./images/collapsed.gif");
		span_title.style.backgroundImage = "url(" + image + ")";
		span_title.style.backgroundRepeat = "no-repeat";
		span_title.style.backgroundPosition = "left center";
		element_vars.style.display = (element_vars.style.display == "none" ? "block" : "none");
	}
}

/*!
 * Display the selected profile.
 * \fn void displayProfile(selected)
 * \param selected Element The profile index to display.
 */
function displayProfile(selected) {
	// Get all the modules sections from all the profiles
	var div_modules_profiles = document.getElementById("modules_profile");
	var modules_profiles = XML.getChildNodes(div_modules_profiles);
	
	// Get all the networks sections from all the profiles
	var div_networks_profiles = document.getElementById("networks_profile").getElementsByTagName("div")[0];
	
	// Get the profiles menu
	var menu_profiles = document.getElementById("menu_profiles").getElementsByTagName("span");
	
	// Get the selected profile to display
	var selected_profile = XML.getNodeValue(selected.attributes["title"]);
	
	// Display the selected profile
	for (var i in modules_profiles) {
		var current_profile = modules_profiles[i];
		// If the current profile matches to the selected one..
		if (XML.getNodeValue(current_profile.attributes["id"]) == (selected_profile + "_profile")) {
			current_profile.style.display = "block";
		}
		// Otherwise..
		else {
			current_profile.style.display = "none";
		}
	}
	
	// Activate the selected profile in the menu
	for (var i in menu_profiles) {
		// Only do something on element nodes
		if (menu_profiles[i].nodeType != 1) {
			continue;
		}
		
		// If the current item menu matches to the clicked one..
		if (XML.getNodeValue(menu_profiles[i].attributes["title"]) == selected_profile) {
			menu_profiles[i].className = 'active';
		}
		// Otherwise..
		else {
			menu_profiles[i].className = '';
		}
	}
	
	// Update the name of the current profile
	document.getElementById("current_profile").value = selected_profile;
}

/*!
 * Display the selected mapping.
 * \fn void displayMapping(selected)
 * \param selected Element The mapping to display.
 */
function displayMapping(selected) {
	var div_mappings = document.getElementById('mapping_config');
	var mappings = XML.getChildNodes(div_mappings);
	
	// Get the mappings menu
	var menu_mappings = document.getElementById("mappings").getElementsByTagName("span");
	
	// Display the selected mapping
	for (var i in mappings) {
		// If the current mapping matches to the selected one..
		if (XML.getNodeValue(mappings[i].attributes["id"]) == "Mapping_" + selected ||
				XML.getNodeValue(mappings[i].attributes["id"]) == "Mapping_profiles_" + selected) {
			mappings[i].style.display = "block";
		}
		// Otherwise..
		else {
			mappings[i].style.display = "none";
		}
	}
	
	// Activate the selected mapping in the menu
	for (var i in menu_mappings) {
		// Only do something on element nodes
		if (menu_mappings[i].nodeType != 1) {
			continue;
		}
		
		// If the current item menu matches to the clicked one..
		if (XML.getNodeValue(menu_mappings[i].attributes["title"]) == "Mapping_" + selected) {
			menu_mappings[i].className = 'active';
		}
		// Otherwise..
		else {
			menu_mappings[i].className = '';
		}
	}
}

/*!
 * Log the message in the console.
 * \fn void logMessage(message, alert)
 * \param message String Message to log.
 * \param alerted Boolean [Optional] true to display an alert box, false otherwise.
 */
function logMessage(message, alerted) {
	alerted = (alerted != undefined) ? alerted : true;
	
	// Display an alert box if asked
	if (alerted) {
		alert(message);
	}
	
	// Add the message in the console messages
	document.getElementById('messages').value += message + "\n";
}

/*!
 * Load a configuration file, check its validity and display it.
 * \fn void displayConfigFile(evt)
 * \param evt Event Event.
 */
function displayConfigFile(evt) {
	var config_file = evt.target.files[0];
	var message = "";
	var xml_content = null;

	var reader = new FileReader();
	
	// Reset the view
	View.resetView();
	
	// Treatment to do if the file reading has been exited on success
	reader.onload = (function(theFile) {
		return function(e) {
			// Get the content (as a string) of the opened config file
			xml_content = e.target.result;
			
			// Use xmllint to validate the opened config file
			var xmllint = validateXML(xml_content, sctk_runtime_config_xsd);
			// If the config file is not valid..
			if (xmllint.indexOf("fails") != -1 || xmllint.indexOf("error") != -1) {
				// Log an error message
				message = "[Error] The given config file \"" + theFile.name +
					"\" is not valid.. Abort!\n\t" + xmllint;
				logMessage(message);
				
				// Reset the view
				View.resetView();
			}
			// Otherwise..
			else {
				message = "[Info] The given config file \"" + theFile.name + "\" is valid.";
				logMessage(message, false);
				
				// Translate the config file into a DOM javascript object 
				config = XML.StringToXML(xml_content);
				
				// Create the data model
				myModel = Model.createProfilesModel(config);
				
				// Create the networks
				myNetworks = Model.createNetworksModel(config);
				
				// Create the mappings
				myMappings = Model.createMappingsModel(config);
				
				// Create the associated view
				View.updateView("default", 0);

				// Disable components for the "default" profile which cannot be renamed
				document.getElementById("current_profile").disabled = false;
				document.getElementById("change_name_btn").disabled = false;				
			}
		}
	})(config_file);
	
	// Treatment to do if the file reading has been exited on error
	reader.onerror = function(stuff) {
		message = "[Error] The given config file \"" + theFile.name + "\" cannot be read.";
		logMessage(message);
	}

	reader.readAsText(config_file);	
}

/*!
 * Check the validity of the generated XML and save it.
 * \fn void saveXMLFile()
 */
function saveXMLFile() {
	var console = document.getElementById('messages');
	var generated_file = document.getElementById("generated_file").value;
	var message = "";

	// Translate the XML generated file into a DOM javascript object
	var xml_generated_file = XML.StringToXML(generated_file);

	// Use xmllint to validate the generated config file
	var xmllint = validateXML(generated_file, sctk_runtime_config_xsd);
	// If the config file is not valid..
	if (xmllint.indexOf("fails") != -1 || xmllint.indexOf("error") != -1) {
		// This should never happen!
		//If so, check the Model.generateXmlConfig() function
		message = "[Error] The generated XML is not valid..\n\t" + xmllint;
		logMessage(message);
	}
	// Otherwise..
	else {
		var bb = new BlobBuilder;
		bb.append(generated_file);
		var blob = bb.getBlob("text/xml");
		saveAs(blob, "config-new.xml");
		message = "[Info] Saving the generated XML...";
		logMessage(message, false);
	}
}

/*!
 * Function to add event listener at the loading of the editor
 * \fn void load()
 */
function load() {
	document.getElementById('config_xml').addEventListener('change', displayConfigFile, false);
}
