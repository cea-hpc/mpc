/*
 * Functions helpers to create the data model.
 */
var Model = {};

Model.createStructModel = function(xml, struct_js) {
	var childs_js = struct_js.childs;
	var childs_keys = Object.keys(childs_js);
	var hh_struct = {};

	for (var i in childs_keys) {
		var current_child_mode = childs_js[childs_keys[i]].mode;
		var hh = {};

		hh = Model.createModel[current_child_mode](xml, childs_js[childs_keys[i]]);

		hh_struct[childs_keys[i]] = hh;
	}

	return hh_struct;
};

Model.createParamModel = function(xml, param_js) {
	var param_attributes = Object.keys(param_js);
	var hh_param = {};

	for (var i in param_attributes) {
		hh_param[param_attributes[i]] = param_js[param_attributes[i]];
	}

	var param_xml = XML.hasChild(xml, param_js["name"]);
	if (isBasicType(param_js["type"]) || Object.keys(meta.enum).indexOf(param_js["type"]) != -1) {
		hh_param["value"] = (param_xml ? XML.getNodeValue(param_xml) : param_js["default"]);
	}
	else if(Object.keys(meta.types).indexOf(param_js["type"]) != -1) {
		hh_param["value"] = Model.createModel[meta.types[param_js["type"]].type](param_xml, meta.types[param_js["type"]]);
	}

	return hh_param;
};

Model.createUnionModel = function(xml, union_js) {
	var choice_js = union_js.choice;
	var choice_keys = Object.keys(choice_js);
	var hh_union = {};

	var union_xml = XML.getChildNodes(xml);
	union_xml = union_xml ? union_xml[0] : union_xml;

	if (union_xml != null) {
		for (var i in choice_keys) {
			if (choice_js[choice_keys[i]].name == union_xml.nodeName) {
				union_js = choice_js[choice_keys[i]];
			}
		}
	}
	else {
		var choices = []
		for (var i in choice_keys) {
			choices.push(choice_keys[i]);
		}
		
		var type = "";
		while (choices.indexOf(type) == -1) {
			type = window.prompt("Please select the type you want to create (" + choices.join(', ') + ")", 
					choice_keys[0]);
			
			if (choices.indexOf(type) == -1) {
				var message = "[Error] Please choose a correct value in [" + choices.join(', ') + "]!";
				logMessage(message, true);
			}
		}
		
		union_js = meta.types[choice_js[type].type];
	}
	
	if (union_js.mode) {
		hh_union = Model.createModel[union_js.mode](union_xml, union_js);
	}
	else if(isBasicType(union_js["type"])) {
		hh_union = Model.createModel[union_js.type](union_xml, union_js);
	}
	else if(Object.keys(meta.types).indexOf(union_js["type"]) != -1) {
		hh_union = Model.createModel[meta.types[union_js["type"]].type](union_xml, meta.types[union_js["type"]]);
	}
	
	return hh_union;
};

Model.createArrayModel = function(xml, array_js) {
	var array_attributes = Object.keys(array_js);
	var hh_array = {};	

	for (var i in array_attributes) {
		//if (array_attributes[i] != "entryname") {
			hh_array[array_attributes[i]] = array_js[array_attributes[i]];
		//}
	}
	
	var array_xml = XML.hasChild(xml, array_js["name"]);
	if (array_xml) {
		var childnodes = XML.getChildNodes(array_xml);
		var array = [];
		
		for (var j in childnodes) {
			if(Object.keys(meta.types).indexOf(array_js["type"]) != -1) {
				array.push(Model.createModel[meta.types[array_js["type"]].type](childnodes[j], meta.types[array_js["type"]]));
			}
			else {
				if (childnodes[j].nodeName == array_js["entryname"]) {
					array.push({type: array_js["type"], value:XML.getNodeValue(childnodes[j])});
				}
			}
		}
		hh_array["values"] = array;
	}
	
	return hh_array;
};

Model.createModulesModel = function(xml) {
	var modules_keys = Object.keys(meta.modules);
	var hh_modules = {};

	for (var i in modules_keys) {
		var module_name = meta.modules[modules_keys[i]].name;
		var module_type = meta.modules[modules_keys[i]].type;
		var module_js = meta.types[module_type];
		var hh_variables = {};

		var module_xml = XML.hasChild(xml, module_name);
		if (module_xml) {
			hh_variables = Model.createModel[module_js.type](module_xml, module_js);
		}

		hh_modules[module_name] = hh_variables;
	}

	return hh_modules;
};

Model.createNetworksModel = function(config) {
	var list_profiles = XML.getChildNodes(XML.getNodes(config, "profiles")[0]);
	var networks_keys = Object.keys(meta.networks);
	var hh_networks = {};
	var xml;
	
	for (var i in list_profiles) {
		var list_elements = XML.getChildNodes(list_profiles[i]);
		
		for (var j in list_elements) {
			if (list_elements[j].nodeName == "name" && XML.getNodeValue(list_elements[j]) == "default") {
				xml = list_profiles[i];
				break;
			}
		}
	}
	
	for (var i in networks_keys) {
		var networks_name = meta.networks[networks_keys[i]].name;
		var networks_type = meta.networks[networks_keys[i]].type;
		var networks_js = meta.types[networks_type];
		var hh_variables = {};

		var networks_xml = XML.hasChild(xml, networks_name);
		if (networks_xml) {
			hh_variables = Model.createModel[networks_js.type](networks_xml, networks_js);
		}

		hh_networks[networks_name] = hh_variables;
	}
	
	return hh_networks;
};

Model.createProfilesModel = function(config) {
	var list_profiles = XML.getChildNodes(XML.getNodes(config, "profiles")[0]);
	var hh_profiles = {};
	
	for (var i in list_profiles) {
		var list_elements = XML.getChildNodes(list_profiles[i]);
		var name = "";
		var hh_modules = {};
		
		for (var j in list_elements) {
			
			if (list_elements[j].nodeName == "name") {
				name = XML.getNodeValue(list_elements[j]);
			}
			else if (list_elements[j].nodeName == "modules") {
				hh_modules = Model.createModulesModel(list_elements[j]);
			}
			else if (list_elements[j].nodeName == "networks") {
				//hh_networks = Model.createNetworksModel(list_profiles[i]);
			}
			else {
				alert("[General] Not implemented: " + list_elements[j].nodeName + ".");
			}
		}
		
		hh_profiles[name] = {modules: hh_modules};
	}
	
	// Update missing values with default ones.
	var profile_names = Object.keys(hh_profiles);
	for (var i in profile_names) {
		if (profile_names[i] == "default") {
			continue;
		}
		
		var module_names = Object.keys(hh_profiles[profile_names[i]].modules);
		for (var j in module_names) {
			if (Object.keys(hh_profiles[profile_names[i]].modules[module_names[j]]).length == 0) {
				hh_profiles[profile_names[i]].modules[module_names[j]] = hh_profiles["default"].modules[module_names[j]];
			}
		}
	}
	
	return hh_profiles;
};

Model.createMappingsModel = function(config) {
	var list_mappings = XML.getChildNodes(XML.getNodes(config, "mappings")[0]);
	var hh_mappings = {};
	
	for (var i in list_mappings) {
		var list_elements = XML.getChildNodes(list_mappings[i]);
		var hh_selectors = {};
		var hh_profiles = {};
		
		for (var j in list_elements) {
			
			if (list_elements[j].nodeName == "selectors") {
				var list_selectors = XML.getChildNodes(list_elements[j]);
				
				for (var k in list_selectors) {
					hh_selectors[XML.getNodeValue(list_selectors[k].attributes["name"])] = 
						{type : list_selectors[k].nodeName, value: XML.getNodeValue(list_selectors[k])};
				}
			}
			else if (list_elements[j].nodeName == "profiles") {
				var list_profiles = XML.getChildNodes(list_elements[j]);
				
				for (var k in list_profiles) {
					var value = XML.getNodeValue(list_profiles[k]);
					hh_profiles[value] = value;
				}
			}
			else {
				alert("[Mappings] Not implemented: " + list_elements[j].nodeName + ".");
			}
		}
		
		hh_mappings[i] = {selectors: hh_selectors, profiles: hh_profiles};
	}
	
	return hh_mappings;
};

Model.updateMappingSelectors = function (name, type_selector, value_selector, index, adding) {
	if (adding) {
		myMappings[index].selectors[name] = {type: type_selector, value: value_selector};
	}
	else {
		delete myMappings[index].selectors[name];
	}
}

Model.updateMappingProfiles = function (profile, index, adding) {
	if (adding) {
		myMappings[index].profiles[profile] = profile;
	}
	else {
		delete myMappings[index].profiles[profile];
	}
};

/*
 * Create a new cli_option in the network configuration section.
 * \fn void createNewCliOption(empty)
 * \param Boolean empty true to create an empty cli_option, false otherwise.
 */
Model.createNewCliOption = function(cli_option_name, empty) {
	empty = empty ? empty : true;
	var cli_options = myNetworks.networks.cli_options;
	var cli_option_type = cli_options.type;
	var cli_option_meta = meta.types[cli_option_type];
	var cli_option_keys = Object.keys(cli_option_meta.childs);
	var hh_new_cli_option = {};
	var idx = cli_options.values ? Object.keys(cli_options.values).length : 0;
	
	hh_new_cli_option = Model.createModel[cli_option_meta.type](null, cli_option_meta);
	hh_new_cli_option.name.value = cli_option_name;
	
	// If no cli_options exists yet, create the values section
	if (idx == 0) {
		myNetworks.networks.cli_options["values"] = {};
	}
	
	myNetworks.networks.cli_options.values[idx] = hh_new_cli_option;
	
	return idx;
};

/*
 * Create a new rail in the network configuration section.
 * \fn void createNewRail(empty)
 * \param Boolean empty true to create an empty rail, false otherwise.
 */
Model.createNewRail = function(rail_name, empty) {
	empty = empty ? empty : true;
	var rails = myNetworks.networks.rails;
	var rail_type = rails.type;
	var rail_meta = meta.types[rail_type];
	var rail_keys = Object.keys(rail_meta.childs);
	var hh_new_rail = {};
	var idx = rails.values ? Object.keys(rails.values).length : 0;
	
	hh_new_rail = Model.createModel[rail_meta.type](null, rail_meta);
	hh_new_rail.name.value = rail_name;
	
	// If no rails exists yet, create the values section
	if (idx == 0) {
		myNetworks.networks.rails["values"] = {};
	}
	
	myNetworks.networks.rails.values[idx] = hh_new_rail;
	
	return idx;
};

/*
 * Create a new driver in the network configuration section.
 * \fn void createNewDriver(empty)
 * \param Boolean empty true to create an empty driver, false otherwise.
 */
Model.createNewDriver = function(driver_name, empty) {
	empty = empty ? empty : true;
	var drivers = myNetworks.networks.configs;
	var driver_type = drivers.type;
	var driver_meta = meta.types[driver_type];
	var driver_keys = Object.keys(driver_meta.childs);
	var hh_new_driver = {};
	var idx = drivers.values ? Object.keys(drivers.values).length : 0;
	
	hh_new_driver = Model.createModel[driver_meta.type](null, driver_meta);
	hh_new_driver.name.value = driver_name;
	hh_new_driver.driver.type = driver_type;
	
	// If no cli_options exists yet, create the values section
	if (idx == 0) {
		myNetworks.networks.drivers["values"] = {};
	}
	
	myNetworks.networks.configs.values[idx] = hh_new_driver;
	
	return idx;
};

/*
 * Update the cli options in the networks section
 */
Model.updateCliOptionsNetworksModel = function(cli_option_name, nb_cli_options, idx) {
	var defined_cli_options = myNetworks.networks.cli_options.values;

	// Update the model network
	for (var i = parseInt(idx); i < nb_cli_options - 1; i ++) {
		defined_cli_options[i] = defined_cli_options[i + 1];
	}
	delete myNetworks.networks.cli_options.values[nb_cli_options - 1];

	// Remove the rail from the available rails
	list_cli_options.splice(list_cli_options.indexOf(cli_option_name), 1);	
};

/*
 * Update the rails in the networks section
 */
Model.updateRailsNetworksModel = function(rail_name, nb_rails, idx) {
	var defined_rails = myNetworks.networks.rails.values;

	// Update the model network
	for (var i = parseInt(idx); i < nb_rails - 1; i ++) {
		defined_rails[i] = defined_rails[i + 1];
	}
	delete myNetworks.networks.rails.values[nb_rails - 1];

	// Remove the rail from the available rails
	list_rails.splice(list_rails.indexOf(rail_name), 1);	

	var cli_options = myNetworks.networks.cli_options.values;
	if (cli_options != undefined) {
		var cli_options_keys = Object.keys(cli_options);	

		// Remove the deleted rail in all the CLI options containing it
		for (var i in cli_options_keys) {
			var rails = cli_options[cli_options_keys[i]].rails.values;
			var rails_keys = Object.keys(rails);

			for (var j in rails_keys) {
				if (rails[rails_keys[j]].value == rail_name) {
					for (var k = parseInt(j); k < rails_keys.length - 1; k ++) {
						rails[rails_keys[k]] = rails[rails_keys[k + 1]];
					}
					delete myNetworks.networks.cli_options.values[cli_options_keys[i]].rails.values[rails_keys[rails_keys.length - 1]];
					break;
				}
			}
		}
	}
};

Model.updateRailsCliOptions = function (cli_options_idx, rail_idx, adding) {
	var cli_options = myNetworks.networks.cli_options.values[cli_options_idx];
	var rails = cli_options.rails;
	var nb_rails = Object.keys(rails.values).length;
	
	if (adding) {
		var selected_rails = document.getElementById('selected_rails_' + cli_options_idx);
		var nb_selected_rails = selected_rails.length;
		var rail_name = selected_rails[nb_selected_rails - 1].value;
		var values = {type: 'string', value: rail_name};
		myNetworks.networks.cli_options.values[cli_options_idx].rails.values[nb_rails] = values;
	}
	else {
		var i = 0;
		for (i = rail_idx; i < nb_rails - 1; i ++) {
			myNetworks.networks.cli_options.values[cli_options_idx].rails.values[i] =
				myNetworks.networks.cli_options.values[cli_options_idx].rails.values[i + 1];
		}
		delete myNetworks.networks.cli_options.values[cli_options_idx].rails.values[i];
	}
};

/*
 * Update the drivers in the networks section
 */
Model.updateDriversNetworksModel = function(driver_name, nb_drivers, idx) {
	var defined_drivers = myNetworks.networks.configs.values;

	// Update the model network
	for (var i = parseInt(idx); i < nb_drivers - 1; i ++) {
		defined_drivers[i] = defined_drivers[i + 1];
	}
	delete myNetworks.networks.configs.values[nb_drivers - 1];

	// Remove the driver from the available drivers
	list_drivers.splice(list_drivers.indexOf(driver_name), 1);	

	var rails = myNetworks.networks.rails.values;
	if (rails != undefined) {
		var rails_keys = Object.keys(rails);	

		// Remove the deleted driver in all the rails containing it
		for (var i in rails_keys) {
			var rail = rails[i];
			if (rail.config.value == driver_name) {
				rail.config.value = "";
			}
		}
	}
};

Model.updateMappingsProfileName = function(old_name, new_name) {
	var mappings = Object.keys(myMappings);
	for (var i in mappings) {
		var profiles = Object.keys(myMappings[mappings[i]].profiles);
		if (profiles.indexOf(old_name) != -1) {
			myMappings[mappings[i]].profiles[new_name] = 
				myMappings[mappings[i]].profiles[old_name];
			delete myMappings[i].profiles[old_name];
		}
	}
};

Model.renameProfile = function(old_name, new_name) {
	myModel[new_name] = myModel[old_name];
	delete myModel[old_name];
};

Model.updateModel = function(profile_name, module_name, var_name, element) {
	var value;
	if (element.type == "checkbox") {
		value = element.checked.toString();
	}
	else {
		value = element.value;
	}
	myModel[profile_name].modules[module_name][var_name].value = value;
}

Model.updateArrayInModel = function(profile_name, module_name, var_name, element) {
	var regexp = new RegExp("[, ]+", "g");
	var values = element.value.split(regexp);
	var array_type = myModel[profile_name].modules[module_name][var_name].type;
	
	myModel[profile_name].modules[module_name][var_name].values = {};
	for (var i = 0; i < values.length; i++) {
		myModel[profile_name].modules[module_name][var_name].values[i] = {type: array_type, value: values[i]};
	}
}

Model.updateSizeInModel = function(profile_name, module_name, var_name) {
	var number = document.getElementById(profile_name + '_' + module_name + '_' + var_name + '_text').value;
	
	var units_cbb = document.getElementById(profile_name + '_' + module_name + '_' + var_name + '_cbb');
	var unit = units_cbb.options[units_cbb.selectedIndex].value;
	
	myModel[profile_name].modules[module_name][var_name].value = number + ' ' + unit;
}

Model.resetValueModel = function(profile_name, module_name, var_name) {
	var default_value = myModel["default"].modules[module_name][var_name].dflt;
	myModel[profile_name].modules[module_name][var_name].value = default_value;
}

Model.createModel = {
		"struct" : Model.createStructModel,
		"param" : Model.createParamModel,
		"union" : Model.createUnionModel,
		"array" : Model.createArrayModel
};

Model.generateXmlConfig = function() {
	var xml = [];
	
	xml.push("<?xml version='1.0'?>");
	xml.push("<mpc xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance' xsi:noNamespaceSchemaLocation='mpc-config.xsd'>");
	xml.push("<profiles>");
	
	
	var profiles_names = Object.keys(myModel);
	for (var i in profiles_names) {
		var current_profile_name = profiles_names[i];
		var current_profile = myModel[current_profile_name];
		xml.push("<profile>");
		xml.push("<name>" + current_profile_name + "</name>");
		
		// modules
		var modules_list = Object.keys(current_profile.modules);
		
		xml.push("<modules>");
		for (var j in modules_list) {
			var current_module_name = modules_list[j];
			var current_module = current_profile.modules[current_module_name];
			
			var elements_list = Object.keys(current_module);
			var xml_elements = [];
			for (var k in elements_list) {
				var current_element_name = elements_list[k];
				var current_element = current_module[current_element_name];
				var current_element_mode = current_element.mode;
				var current_default_value = myModel["default"].modules[current_module_name][current_element_name].value;
				
				if (current_element.value != undefined) {
					if (current_profile_name == "default" 
						|| (current_profile_name != "default" && current_element.value != current_default_value)) {
						xml_elements.push("<" + current_element_name + ">" 
								+ current_element.value 
								+ "</" + current_element_name + ">");
					}
				}
				else if (current_element_mode == "array") {
					if (current_profile_name == "default") {
						xml_elements.push("<" + current_element_name + ">");
						
						var current_element_values = current_element.values;
						var nb_values = Object.keys(current_element_values).length;
						
						for (var l = 0; l < nb_values; l ++) {
							xml_elements.push("<" + current_element.entryname + ">"
									+ current_element_values[l].value
									+ "</" + current_element.entryname + ">");
						}
						
						xml_elements.push("</" + current_element_name + ">");
					}
				}
			}
			
			if (xml_elements.length > 0) {
				xml.push("<" + current_module_name + ">");
				xml = xml.concat(xml_elements);
				xml.push("</" + current_module_name + ">");
			}
		}
		xml.push("</modules>");

		if (current_profile_name == "default") {
			xml.push(Model.generateNetworksXmlConfig());
		}
		
		xml.push("</profile>");
	}
	xml.push("</profiles>");
	
	// Generate the 
	xml.push(Model.generateMappingsXmlConfig());
	
	xml.push("</mpc>");
	
	return XML.formatXML(xml.join('\r\n'));
};

Model.generateNetworksXmlConfig = function() {
	var xml = [];
	var networks = myNetworks.networks;
	var networks_keys = Object.keys(myNetworks.networks);
	
	if (networks_keys.length > 0) {
		xml.push("<networks>"); 
		xml.push(Model.generateStructXML(networks, meta.types.networks));
		xml.push("</networks>")
	}
	
	return xml.join('');
};

Model.generateMappingsXmlConfig = function() {
	var xml = [];
	var xml_mappings = [];
	
	var mappings = Object.keys(myMappings);
	if (mappings.length > 0) {
		for (var i in mappings) {
			var current_mapping = myMappings[mappings[i]];
			var selectors = [];
			var profiles = [];
			
			var list_selectors = Object.keys(current_mapping.selectors);
			for (var j in list_selectors) {
				var current_selector = current_mapping.selectors[list_selectors[j]];
				selectors.push("<" + current_selector.type + " name='" + list_selectors[j] + "'>");
				selectors.push(current_selector.value);
				selectors.push("</" + current_selector.type + ">");
			}
			
			var list_profiles = Object.keys(current_mapping.profiles);
			for (var j in list_profiles) {
				var current_profile = current_mapping.profiles[list_profiles[j]];
				profiles.push("<profile>" + list_profiles[j] + "</profile>");
			}
			
			if (selectors.length > 0 && profiles.length > 0) {
				xml_mappings.push("<mapping>");
				xml_mappings.push("<selectors>");
				xml_mappings = xml_mappings.concat(selectors.join(''));
				xml_mappings.push("</selectors>");
				xml_mappings.push("<profiles>");
				xml_mappings = xml_mappings.concat(profiles.join(''));
				xml_mappings.push("</profiles>");
				xml_mappings.push("</mapping>");
			}
		}
		if (xml_mappings.length > 0) {
			xml.push("<mappings>");
			xml = xml.concat(xml_mappings.join(''));
			xml.push("</mappings>");
		}
	}
	
	return xml.join('');
};

Model.generateStructXML = function(hh_struct, struct_js) {
	var xml = [];
	
	var childs_js = struct_js.childs;
	var childs_keys = Object.keys(childs_js);

	for (var i in childs_keys) {
		var current_child_mode = childs_js[childs_keys[i]].mode;
		var current_child_type = childs_js[childs_keys[i]].type;
		var choice = current_child_mode ? current_child_mode : current_child_type;
		xml.push(Model.generateXML[choice](hh_struct[childs_keys[i]], childs_js[childs_keys[i]]));
	}

	return xml.join('');
};

Model.generateParamXML = function(hh_param, param_js) {
	var xml = [];

	xml.push("<" + hh_param.name + ">");
	xml.push(hh_param.value);
	xml.push("</" + hh_param.name + ">");

	return xml.join('');
};

Model.generateArrayXML = function(hh_array, array_js) {
	var xml = [];
	var values = hh_array.values;
	var values_keys = Object.keys(values);
	
	xml.push("<" + hh_array.name + ">");
	for (var i in values_keys) {
		xml.push("<" + hh_array.entryname + ">");
		
		if (isBasicType(array_js.type)) {
			xml.push(values[values_keys[i]].value);
		}
		else if (Object.keys(meta.types).indexOf(array_js.type) != -1) {
			xml.push(Model.generateXML[meta.types[array_js.type].type](values[values_keys[i]], meta.types[array_js.type]));
		}
		else {
			alert("errooooooooooor : " + array_js.type);
		}
		
		xml.push("</" + hh_array.entryname + ">");
	}
	xml.push("</" + hh_array.name + ">");

	return xml.join('');
};

Model.generateUnionXML = function(hh_union, union_js) {
	var xml = [];
	var values = hh_union.value;
	var values_keys = Object.keys(values);
	
	for (var i in values_keys) {
		if (isBasicType(union_js.type)) {
			xml.push(values[values_keys[i]].value);
		}
		else if (Object.keys(meta.types).indexOf(union_js.type) != -1) {
			xml.push(Model.generateXML[meta.types[union_js.type].type](values[values_keys[i]], meta.types[union_js.type]));
		}
		else {
			alert("errooooooooooor : " + union_js.type);
		}
	}
	xml.push("</" + hh_union.name + ">");

	return xml.join('');
};

Model.generateXML = {
		"struct": Model.generateStructXML,
		"param": Model.generateParamXML,
		"array": Model.generateArrayXML,
		"union": Model.generateUnionXML
}

