/*
 * \file EditorView.js
 * \namespace View
 * Functions helpers to create the data view.
 */
var View = {};

// List of the available CLI options
var list_cli_options = [];

// List of the available rails
var list_rails = [];

// List of the available drivers
var list_drivers = [];

/*
 * Create an HTML object for a XML node with param mode
 * \fn String View.createParamView(profile_name, module_name, var_name, module, print)
 * \param String profile_name Name of the modified profile
 * \param String module_name Name of the modified module
 * \param String var_name Name of the modified property
 * \param Boolean print false for size field, true otherwise
 * \param Boolean network true if it is for a network section, false otherwise
 * \return String The HTML code for the given property
 */
View.createParamView = function (profile_name, module_name, var_name, module, print, network) {
        if(View.createView[module.type] == undefined)
                return

	return View.createView[module.type](profile_name, module_name, var_name, module, print, network);
};

/*
 * Create an HTML input of "text" type
 * \fn String View.createTexteField(profile_name, module_name, var_name, module, print)
 * \param String profile_name Name of the modified profile
 * \param String module_name Name of the modified module
 * \param String var_name Name of the modified property
 * \param Boolean print false for size field, true otherwise
 * \return String The HTML code for the given property
 */
View.createTextField = function (profile_name, module_name, var_name, module, print, network) {
	var html = "";
	network = network ? network : false;
	var title = module.doc ? module.doc : "";
	var re = new RegExp("'", "g");
	title = title.replace(re, "\\'");

	if (print) {
		html += "<tr><th>" + module.name + "</th>";
	}

	html += "<td><input type='text' name='" + profile_name + "_" + module_name + "_" + var_name
		+ "' id='" + profile_name + "_" + module_name + "_" + var_name;
	
	html += "' value='" + ((module.type != "size") ? (module.value ? module.value : module.dflt) 
				 : (module.value ? module.value.match(/\d+/g)[0] : module.dflt.match(/\d+/g)[0]));

	html += "' title='" + title + "\nDefault value: " + module.dflt + "'";
	html += " data-profile='" + profile_name + "'";
	html += " data-module='" + module_name + "'";
	html += " data-property='" + module.name + "'";
	html += " data-default='" + module.dflt + "'";
	
	if (network == true) {
		html += " onchange='Controller.updateNetwork(&quot;" + profile_name + "&quot;, &quot;"
		+ module_name + "&quot;, &quot;" + module.name + "&quot;, this)'/>";
	}
	else {
		if (module.type == "size") {
			html += " onchange='Controller.updateSizeInProfile(&quot;" + profile_name + "&quot;, &quot;"
			+ module_name + "&quot;, &quot;" + module.name + "&quot;)'/>";
		}
		else {
			html += " onchange='Controller.updateProfile(&quot;" + profile_name + "&quot;, &quot;"
			+ module_name + "&quot;, &quot;" + var_name + "&quot;, this)'/>";
		}
	}
	
	if (print) {
		html += "</tr>";
	}

	return html;
};

/*
 * Create an HTML input of "checkbox" type
 * \fn String View.createCheckboxField(profile_name, module_name, var_name, module, print)
 * \param String profile_name Name of the modified profile
 * \param String module_name Name of the modified module
 * \param String var_name Name of the modified property
 * \param Boolean print false for size field, true otherwise
 * \param Boolean network true if it is for a network section, false otherwise
 * \return String The HTML code for the given property
 */
View.createCheckboxField = function (profile_name, module_name, var_name, module, print, network) {
	var html = "";
	network = network ? network : false;
	var title = module.doc ? module.doc : "";
	var re = new RegExp("'", "g");
	title = title.replace(re, "\\'");

	if (print) {
		html += "<tr>";
		html += "<th>" + module.name + "</th>";
	}

	html += "<td><input type='checkbox' name='" + profile_name + "_" + module_name + "_" + var_name + "'";

	if (module.value == "true") {
		html += " checked='true'";
	}

	html += " title='" + title + "\nDefault value: " + module.dflt + "'";
	html += " data-profile='" + profile_name + "'";
	html += " data-module='" + module_name + "'";
	html += " data-property='" + module.name + "'";
	html += " data-default='" + module.dflt + "'";
	
	if (network == true) {
		html += " onchange='Controller.updateNetwork(&quot;" + profile_name + "&quot;, &quot;"
		+ module_name + "&quot;, &quot;" + module.name + "&quot;, this)'/>";
	}
	else {
		html += " onchange='Controller.updateProfile(&quot;" + profile_name + "&quot;, &quot;" + 
				module_name + "&quot;, &quot;" + var_name + "&quot;, this)'/>";
	}
	
	html += "</td>";
	
	if (print) {
		html += "</tr>";
	}
	
	return html;
};

/*
 * Create an HTML "combobox"
 * \fn String View.createComboboxField(profile_name, module_name, var_name, module, print)
 * \param String profile_name Name of the modified profile
 * \param String module_name Name of the modified module
 * \param String var_name Name of the modified property
 * \param Boolean print false for size field, true otherwise
 * \param Boolean network true if it is for a network section, false otherwise
 * \return String The HTML code for the given property
 */
View.createComboboxField = function (profile_name, module_name, var_name, module, print, network) {
	var html = "";
	network = network ? network : false;
	var title = module.doc ? module.doc : "";
	var re = new RegExp("'", "g");
	title = title.replace(re, "\\'");

	if (print) {
		html += "<tr>";
		html += "<th>" + module.name + "</th>"; 
		html += "<td>";
	}

	html += "<select name='" + profile_name + "_" + module_name + "_" + var_name
	+ "' id='" + profile_name + "_" + module_name + "_" + var_name
	+ "' title='" + title + "\nDefault value: " + module.dflt + "'";

	if (network == true) {
		html += " onchange='Controller.updateNetwork(&quot;" + profile_name + "&quot;, &quot;"
		+ module_name + "&quot;, &quot;" + module.name + "&quot;, this)'/>";
	}
	else {
		if (module.type == "size") {
			html += " onchange='Controller.updateSizeInProfile(&quot;" + profile_name + "&quot;, &quot;"
			+ module_name + "&quot;, &quot;" + module.name + "&quot;)'/>";
		}
		else {
			html += " onchange='Controller.updateProfile(&quot;" + profile_name + "&quot;, &quot;"
			+ module_name + "&quot;, &quot;" + var_name + "&quot;, this)'/>";
		}
	}
	
	var values = (module.type == "size") ? ["B", "KB", "MB", "GB", "TB", "PB"] : meta.enum[module.type].values;

	var isSelected = false;

	for (var i in values) {
		html += "<option value='" + values[i] + "'";
		if (values[i] == (module.value ? module.value.match(/[a-zA-Z]+/g)[0] : module.dflt.match(/[a-zA-Z]+/g)[0])) {
			isSelected = true;
			html += " selected=\"selected\"";
		}
		else if(values[i] == module.value)
		{
			isSelected = true;
			html += " selected=\"selected\"";	
		}

		html += ">" + values[i] + "</option>";
	}

	html += "<option value='undefined'" + (isSelected == false ? " selected" : "") + ">undefined</option>";
	html += "</select></td>";
	
	if (print) {
		html += "</tr>";
	}

	return html;
};

/*
 * Create HTML "text" + "combobox" for size property
 * \fn String View.createSizeField(profile_name, module_name, var_name, module, print)
 * \param String profile_name Name of the modified profile
 * \param String module_name Name of the modified module
 * \param String var_name Name of the modified property
 * \param Boolean print false for size field, true otherwise
 * \param Boolean network true if it is for a network section, false otherwise
 * \return String The HTML code for the given property
 */
View.createSizeField = function (profile_name, module_name, var_name, module, print, network) {
	var html = "";
	network = network ? network : false;
	var title = module.doc ? module.doc : "";
	var re = new RegExp("'", "g");
	title = title.replace(re, "\\'");

	if (print) {
		html += "<tr>";
		
		html += "<th>" + var_name + "</th>";
	}

	html += View.createTextField(profile_name, module_name, var_name + "_text", module, false, network); 
	html += View.createComboboxField(profile_name, module_name, var_name + "_cbb", module, false, network);
	
	if (print) {
		html += "</tr>";
	}

	return html;
};

/*
 * Create HTML "text" for array property
 * \fn String View.createArrayField(profile_name, module_name, var_name, module, print)
 * \param String profile_name Name of the modified profile
 * \param String module_name Name of the modified module
 * \param String var_name Name of the modified property
 * \param Boolean print false for size field, true otherwise
 * \param Boolean network true if it is for a network section, false otherwise
 * \return String The HTML code for the given property
 */
View.createArrayField = function (profile_name, module_name, var_name, module, print, network) {
	var html = "";
	network = network ? network : false;
	var title = module.doc ? module.doc : "";
	var re = new RegExp("'", "g");
	title = title.replace(re, "\\'");

	if (print) {
		html += "<tr>";
		html += "<th>" + var_name + "</th>";
	}

	html += "<td><input type='text' name='" + profile_name + "_" + module_name + "_" + var_name;
	
	var values = "";
	for (var i in Object.keys(module.values)) {
		values += module.values[i].value;
		if (i < Object.keys(module.values).length - 1) {
			values += ", ";
		}
	}
	
	html += "' value='" + values + "'";

	html += " title='" + title + "\nDefault value: " + module.dflt + "'";
	html += " data-default='" + module.dflt + "'";
	
	if (network == true) {
		html += " onchange='Controller.updateNetwork(&quot;" + profile_name + "&quot;, &quot;"
		+ module_name + "&quot;, &quot;" + module.name + "&quot;, this)'/>";
	}
	else {
		html += " onchange='Controller.updateArrayInProfile(&quot;" + profile_name + "&quot;, &quot;" + 
				module_name + "&quot;, &quot;" + var_name + "&quot;, this)'/>";
	}
	
	html += "<span class='note'>Enter values separated with comma</span>"
	html += "</td>";
	
	if (print) {
		html += "</tr>";
	}
	
	return html;
};

/*
 * Create the view for a module.
 * \fn String View.createModulesView(name, hh_modules)
 * \param name String Name of the module.
 * \param hh_modules Object Hash table containing all the data for the module
 * \return String The HTML code
 */
View.createModulesView = function (name, hh_modules) {
	var modules_keys = Object.keys(hh_modules);
	var output = [];
	
	for (var i in modules_keys) {
		var hh_module = hh_modules[modules_keys[i]];
		var module_elements = Object.keys(hh_module);
		
		output.push("<table class='config-module'><thead>");
		output.push("<th colspan='3' id='" + name + "_" + modules_keys[i].toLowerCase() 
				+ "' onclick='printElement(this)'><span>" + modules_keys[i].toUpperCase() 
				+ "</span></th>");
		output.push("</thead><tbody id='" + name + "_" + modules_keys[i].toLowerCase() 
				+ "_vars' style='display:none;'>");
		
		for (var j in module_elements) {
			var html_module = [];
			
			var select_view_constructor = hh_module[module_elements[j]].mode ? hh_module[module_elements[j]].mode 
					: hh_module[module_elements[j]].type;

			html_module.push(View.createView[select_view_constructor](name, modules_keys[i],
					module_elements[j], hh_module[module_elements[j]], true));
			
			output.push(html_module.join(''))
		}

		output.push("</tbody></table>");
	}

	return output.join('');
};

/*
 * Update the view for CLI options.
 * \fn String View.updateCliOptionsView()
 * \return String The HTML code
 */
View.updateCliOptionsView = function() {
	var output = [];
	
	var cli_options = myNetworks.networks.cli_options;
	if (cli_options != undefined && cli_options.values != undefined) {
		var cli_options_keys = Object.keys(cli_options.values);
		for (var j in cli_options_keys) {
			output.push("<tr><td><span class='network-th' onclick='View.printCliOption(&quot;" + cli_options_keys[j] + "&quot;, " +
			"&quot;network-print&quot;, this);'>");
			output.push(cli_options.values[cli_options_keys[j]].name.value);

			// Only add the cli_option if it does not exist in the list yet
			if (list_cli_options.indexOf(cli_options.values[cli_options_keys[j]].name.value) == -1) {
				list_cli_options.push(cli_options.values[cli_options_keys[j]].name.value);			
			}

			output.push("</span></td></tr>");
		}
	}
	
	return output.join('');
};

/*
 * Update the view for rails.
 * \fn String View.updateRailsView()
 * \return String The HTML code
 */
View.updateRailsView = function() {
	var output = [];
	var rails = myNetworks.networks.rails;
	
	if (rails != undefined && rails.values != undefined) {
		var rails_keys = Object.keys(rails.values);
		for (var j in rails_keys) {
			output.push("<tr><td><span class='network-th' onclick='View.printRail(&quot;" + rails_keys[j] + "&quot;, " +
			"&quot;network-print&quot;, this);'>");
			output.push(rails.values[rails_keys[j]].name.value);

			// Only add the rails if it does not exist in the list yet
			if (list_rails.indexOf(rails.values[rails_keys[j]].name.value) == -1) {
				list_rails.push(rails.values[rails_keys[j]].name.value);			
			}

			output.push("</span></td></tr>")
		}
	}
	
	return output.join('');
};

/*
 * Update the view for drivers.
 * \fn String View.updateDriversView()
 * \return String The HTML code
 */
View.updateDriversView = function() {
	var output = [];
	var drivers = myNetworks.networks.configs;
	
	// If drivers existed in the configuration file
	if (drivers != undefined && drivers.values != undefined) {
		var drivers_keys = Object.keys(drivers.values);
		for (var j in drivers_keys) {
			output.push("<tr><td><span class='network-th' onclick='View.printDriver(&quot;" + drivers_keys[j] + "&quot;, " +
			"&quot;network-print&quot;, this);'>");
			output.push(drivers.values[drivers_keys[j]].name.value);

			// Only add the driver if it does not exist in the list yet
			if (list_drivers.indexOf(drivers.values[drivers_keys[j]].name.value) == -1) {
				list_drivers.push(drivers.values[drivers_keys[j]].name.value);
			}

			output.push("</span></td></tr>")
		}
	}
	
	return output.join('');
};

/*
 * Create the view for the networks.
 * \fn String View.createNetworksView(hh_networks)
 * \param hh_networks Object Hash table containing all the data for the networks
 * \return String The HTML code
 */
View.createNetworksView = function (hh_networks) {
	var output = [];
	hh_networks = hh_networks.networks;

	output.push("<div id='networks_profile'>");
	output.push("<table>");
	output.push("<tbody><tr><td id='menu_networks'><div class='overflow'>");

	output.push("<table id='networks' class='config-networks'>");
	output.push("<tbody><tr><td>");

	// CLI options
	output.push("<table class='config-network'>");
	output.push("<thead><tr><th id='cli_options'>");
	output.push("<span onclick='printElement(\"cli_options\")'>CLI OPTIONS</span>");
	output.push("<img src='./images/plus.png' alt='Create a new CLI option' title='Create a new CLI option' " +
			"style='padding-left:5px;cursor:pointer;' onclick='Controller.addCliOption();'/>");
	output.push("<img src='./images/minus.png' alt='Delete current CLI option' title='Delete current CLI option' " +
			"style='padding-left:5px;cursor:pointer;' onclick='Controller.delCliOption();'/>");
	output.push("</th></tr></thead>");
	output.push("<tbody id='cli_options_vars'>");
	
	output.push(View.updateCliOptionsView());

	output.push("</tbody>");
	output.push("</table>");

	// Rails definition
	output.push("<table class='config-network'>");
	output.push("<thead><tr><th id='rails'>");
	output.push("<span onclick='printElement(\"rails\")'>RAILS</span>");
	output.push("<img src='./images/plus.png' alt='Create a new rail' title='Create a new rail' " +
			"style='padding-left:5px;cursor:pointer;' onclick='Controller.addRail(); printElement(\"rails\");'/>");
	output.push("<img src='./images/minus.png' alt='Delete current rail' title='Delete current rail' " +
			"style='padding-left:5px;cursor:pointer;' onclick='Controller.delRail();'/>");
	output.push("</th></tr></thead>");
	output.push("<tbody id='rails_vars' style='display:none'>");
	
	output.push(View.updateRailsView());

	output.push("</td></tr></tbody>");
	output.push("</table>");

	// Drivers definition
	output.push("<table class='config-network'>");
	output.push("<thead><tr><th id='drivers'>");
	output.push("<span onclick='printElement(\"drivers\")'>DRIVERS</span>");
	output.push("<img src='./images/plus.png' alt='Create a new driver' title='Create a new driver' " +
			"style='padding-left:5px;cursor:pointer;' onclick='Controller.addDriver();'/>");
	output.push("<img src='./images/minus.png' alt='Delete current driver' title='Delete current driver' " +
			"style='padding-left:5px;cursor:pointer;' onclick='Controller.delDriver();'/>");
	output.push("</th></tr></thead>");
	output.push("<tbody id='drivers_vars' style='display:none'>");
	
	output.push(View.updateDriversView());

	output.push("</td></tr></tbody>");
	output.push("</table>");

	output.push("</td></tr></tbody>");
	output.push("</table>")

	output.push("</div></td><td style='overflow:auto'>");
	output.push("<div id='network-print'></div>");
	output.push("</td></tr></tbody></table>");

	output.push("</div><br/>");

	return output.join('');
};

/*
 * Print a CLI option
 * \fn void View.printCliOption(cli_options_idx, element, clicked_element)
 * \param cli_options_idx Integer CLI option index to display
 * \param element Element Id of where to display this CLI option.
 * \param clicked_element Element If not null, HTML matching to this CLI option
 */
View.printCliOption = function(cli_options_idx, element, clicked_element) {
	var hh_networks = myNetworks.networks;
	var name = hh_networks.cli_options.values[cli_options_idx].name.value;
	var cli_options = hh_networks.cli_options.values[cli_options_idx].rails;
	var output = [];
	
	// Make the list of the available rails
	var selected_rails = [];
	
	if (cli_options.values != undefined) {
		var rails = Object.keys(cli_options.values);
		for (var i in rails) {
			selected_rails.push(cli_options.values[rails[i]].value);
		}
	}
	
	output.push("<table class='config-cli_options'>");
	output.push("<thead><tr><th id='test1' colspan='2' onclick='printElement(this)'>")
	output.push("<span>CLI options: " + name + "</span>");
	output.push("</th><tr></thead>");
	output.push("<tbody id='test1_vars'>");

	output.push("<table style='text-align:center'><thead>");
	output.push("<tr><th>Available rails</th><th>Selected rails</th></tr>");
	output.push("</thead><tbody><tr>");
	output.push("<td>");
	output.push("<select id='available_rails_" + cli_options_idx + "' multiple style='width:200px;height:100px;'>");
	
	// Create the available rails section
	for (var i in list_rails) {
		if (selected_rails.indexOf(list_rails[i]) == -1) {
			output.push("<option value='" + list_rails[i] + "'" +
					" ondblclick='View.printRail(&quot;" + i +"&quot;, &quot;add-rail&quot;, null);this.selected = false;'" +
					">" + list_rails[i] + "</option>");
		}
	}
	
	output.push("</select>");
	output.push("</td><td>");
	output.push("<select id='selected_rails_" + cli_options_idx + "' multiple style='width:200px;height:100px;'>");
	
	
	// Create the selected rails section
	for (var i in selected_rails) {
		output.push("<option value='" + selected_rails[i] + "'" +
				" ondblclick='View.printRail(&quot;" + list_rails.indexOf(selected_rails[i]) + "&quot;, " +
				"&quot;add-rail&quot;, null);this.selected = false;'" +
				">" + selected_rails[i] + "</option>")
	}
	
	output.push("</td>");
	output.push("</tr></tbody><tfoot><tr>");
	output.push("<td>");
	output.push("<input type='button' value='->' onclick='Controller.selectRailCliOptions(" + cli_options_idx + ")'/>");
	output.push("</td>");
	output.push("<td>");
	output.push("<input type='button' value='<-' onclick='Controller.unselectRailCliOptions(" + cli_options_idx + ")'/>");
	output.push("<input type='button' value='Add new' onclick='Controller.addRail(" + cli_options_idx + ", true)'/>");
	output.push("</td>");
	output.push("</tr></tfoot></table>");
	
	output.push("</tbody></table>");
	output.push("<div id='add-rail'></div>");

	// Change the color of the element to identify it in the menu
	if (clicked_element != null) {		
		var network_th = XML.getElementsByClassName('network-th');
		for (var i in network_th) {
			network_th[i].style.color = "#006666";
		}
		clicked_element.style.color = "red";
	}
	
	document.getElementById(element).innerHTML = output.join('');
}

/*
 * Print a rail
 * \fn void View.printRail(rail_idx, element, clicked_element)
 * \param rail_idx Integer Rail index to display
 * \param element Element Id of where to display this rail.
 * \param clicked_element Element If not null, HTML matching to this rail
 */
View.printRail = function(rail_idx, element, clicked_element) {
	var hh_networks = myNetworks.networks;
	var name = hh_networks.rails.values[rail_idx].name.value;
	var rail_elements = hh_networks.rails.values[rail_idx];
	var output = [];
	var list_rail_elements = Object.keys(rail_elements);

	output.push("<table class='config-rail'>");
	output.push("<thead><tr><th id='test2' colspan='2' onclick='printElement(this)'>")
	output.push("<span>Rail: " + name + "</span>");
	output.push("</th><tr></thead>");
	output.push("<tbody id='test2_vars'>");
	
	for (var i in list_rail_elements) {
		if (rail_elements[list_rail_elements[i]].name != 'name') {
			if (rail_elements[list_rail_elements[i]].name != 'config') {
				if (isBasicType(rail_elements[list_rail_elements[i]].type)) {
					output.push(View.createView[rail_elements[list_rail_elements[i]].type]("rails", rail_idx,
							list_rail_elements[i], rail_elements[list_rail_elements[i]], true, true));
				}
				else if (Object.keys(meta.enum).indexOf(rail_elements[list_rail_elements[i]].type) != -1) {
					output.push(View.createComboboxField("rails", rail_idx, list_rail_elements[i], 
							rail_elements[list_rail_elements[i]], true, true));
				}
			}
			else {
				var output_config = [];
				var id_select = "driver_selection";
				var selected = false;

				output_config.push("<tr><th>" + rail_elements[list_rail_elements[i]].name + "</th><td>");
				output_config.push("<select id='" + id_select + "' " +
						"onchange='View.printDriverThroughRail(\"" + id_select + "\");" +
								"Controller.updateNetwork(&quot;rails&quot;, " + rail_idx + ", &quot;" 
								+ rail_elements[list_rail_elements[i]].name + "&quot;, this.options[this.selectedIndex])'>");
				for (var j in list_drivers) {
					output_config.push("<option value='" + list_drivers[j] + "'" 
							+ (list_drivers[j] == rail_elements[list_rail_elements[i]].value ? " selected" : "")
							+ ">" + list_drivers[j] + "</option>");
					selected = selected || (list_drivers[j] == rail_elements[list_rail_elements[i]].value);
				}
				output_config.push("<option value='undefined'" + (selected == true ? "" : " selected") + ">undefined</option>");
				output_config.push("</select>");
				output_config.push("<img src='./images/plus.png' alt='edit' " +
						"style='padding-left:5px;cursor:pointer;' " +
						"onclick='Controller.addDriver(\"" + id_select + "\", true);'/>");
				output_config.push("<img src='./images/edit.png' alt='edit' " +
						"style='padding-left:5px;cursor:pointer;'' " +
						"onclick='View.printDriverThroughRail(\"" + id_select + "\");'/>");
				output_config.push("</td></tr>");

				output.push(output_config.join(''));
			}
		}
	}
	
	output.push("</tbody></table>");
	output.push("<div id='add-driver'></div>");

	if (clicked_element != null) {
		var network_th = XML.getElementsByClassName('network-th');
		for (var i in network_th) {
			network_th[i].style.color = "#006666";
		}
		clicked_element.style.color = "red";
	}
	
	document.getElementById(element).innerHTML = output.join('');
};

/*
 * Print a driver through a rail
 * \fn void View.printDriverThroughRail(choices_id)
 * \param choices_id Integer Index of the rail to display.
 */
View.printDriverThroughRail = function(choices_id) {
	var choices = document.getElementById(choices_id);
	View.printDriver(choices.selectedIndex, "add-driver", null);
};

/*
 * Print a driver
 * \fn void View.printDriver(driver_name, element, clicked_element)
 * \param driver_name Integer Driver index to display
 * \param element Element Id of where to display this driver.
 * \param clicked_element Element If not null, HTML matching to this driver
 */
View.printDriver = function(driver_name, element, clicked_element) {
	var output = [];
	var drivers = myNetworks.networks.configs.values;
	
	if (drivers[driver_name] != undefined) {
		var name = drivers[driver_name].name.value;
		var driver = drivers[driver_name].driver;
		var list_elements = Object.keys(driver.value.childs);

		output.push("<table class='config-driver'>");
		output.push("<thead><tr><th id='test3' colspan='2' onclick='printElement(this)'>")
		output.push("<span>Driver: " + name + "</span>");
		output.push("</th><tr></thead>");
		output.push("<tbody id='test3_vars'>");

		for (var i in list_elements) {
			if (isBasicType(driver.value.childs[list_elements[i]].type)) {
				output.push(View.createView[driver.value.childs[list_elements[i]].type]("configs", driver_name,
						list_elements[i], driver.value.childs[list_elements[i]], true, true));
			}
			else if (Object.keys(meta.enum).indexOf(driver.value.childs[list_elements[i]].type) != -1) {
				output.push(View.createComboboxField("configs", driver_name, list_elements[i], 
						driver.value.childs[list_elements[i]], true, true));
			}
		}
		output.push("</tbody></table>");

		if (clicked_element != null) {
			var network_th = XML.getElementsByClassName('network-th');
			for (var i in network_th) {
				network_th[i].style.color = "#006666";
			}
			clicked_element.style.color = "red";
		}
	}
	else {
		var message = "[Error] Nothing to display for this driver, assure you that it is defined!";
		logMessage(message, true);
	}
	
	document.getElementById(element).innerHTML = output.join('');
};

/*
 * Create the view for the profiles
 * \fn Object View.createProfilesView = function(hh_profiles, activated)
 * \param hh_profiles Object Hash table containing all the data profiles
 * \param activated String Name of the profile to display
 * \return Object An hash containing the HTML code/
 */
View.createProfilesView = function(hh_profiles, activated) {
	var list_profiles = Object.keys(hh_profiles);
	var menu_profiles = [];
	var html_profiles = [];
	var view = {};
	
	menu_profiles.push("<img class='button' src='./images/add_button.png' " +
			"title='Add a new profile' onclick='Controller.addProfile()'/>");
	menu_profiles.push("<img class='button' src='./images/del_button.png' " +
			"title='Delete current profile' onclick='Controller.delProfile()'></div>");
	menu_profiles.push("<ul>");
	html_profiles.push("<div id='modules_profile'>");
	
	for (var i in list_profiles) {
		var html_profile = [];
		
		menu_profiles.push("<li><span title='" + list_profiles[i] + "'"
						+ (list_profiles[i] == activated ? " class='active'" : "")
						+ " onclick='displayProfile(this);'>" + list_profiles[i] + "</span></li>");
		
		var html = "<table id='" + list_profiles[i] + "_profile' class='config-profile' style='display:"
				+ (list_profiles[i] != activated ? "none" : "block") + "'><tr><td>" 
				+ View.createModulesView(list_profiles[i], hh_profiles[list_profiles[i]].modules)
				+ "</td></tr></table>";
		html_profiles.push(html);
	}

	menu_profiles.push("</ul></div>");
	menu_profiles.push("<br/>");

	view["menu_profiles"] = menu_profiles.join('');
	view["modules"] = html_profiles.join('');
	
	return view;
};

/*
 * Create the view for a selector of env type
 * \fn View.createEnvSelectorView(name, value, parent, line_nb)
 * \param name String Name of the selector
 * \param value String Value of the selector
 * \param parent Integer Index of the mapping containing this selector
 * \param line_nb Integer Index of the selector in the mapping
 * \return String The HTML code.
 */
View.createEnvSelectorView = function(name, value, parent, line_nb) {
	var selector = [];
	var id_type = "selector_type_" + parent + "_" + line_nb;
	var id_name = "selector_name_" + parent + "_" + line_nb;
	var id_value = "selector_value_" + parent + "_" + line_nb;

	selector.push("<tr>");
	selector.push("<td>");
	selector.push("<select id='" + id_type + "'><option value='env' selected>Environment</option></select>");
	selector.push("</td>");
	selector.push("<td>");
	selector.push("<input id='" + id_name + "' type='text' value='" + name + "' " +
			"onfocus='this.oldvalue = this.value' " +
			"onchange='Controller.updateSelectorName(" + parent + ", " + line_nb + ", this)'/>");
	selector.push("&nbsp;=&nbsp;");
	selector.push("<input id='" + id_value + "' type='text' value='" + value + "' " +
			"onchange='Controller.updateSelectorValue(" + parent + ", " + line_nb + ", this)'/>");
	selector.push("</td>");
	selector.push("<td><img src='./images/delete.png' title='Delete selector' " +
			"onclick='Controller.delSelector(" + parent + ", &quot;" + line_nb + "&quot;)'/></td>");
	selector.push("</tr>");
	
	return selector.join('');
};

/*
 * Create the view for the mappings
 * \fn String View.createMappingsView = function(hh_mappings, current_mapping)
 * \param hh_mappings Object Hash table containing all the data mappings
 * \param current_mapping String Name of the mapping to display
 * \return String The HTML code
 */
View.createMappingsView = function(hh_mappings, current_mapping) {
	var list_mappings = Object.keys(hh_mappings);
	var mappings = [];
	var view = {};
	
	mappings.push("<table><tbody>");
	
	mappings.push("<tr><td style='width:70px;text-align:center;vertical-align:top;'>");
	mappings.push("<img class='button' src='./images/add_button.png' " +
			"title='Add a new mapping' onclick='Controller.addMapping();'/>");
	mappings.push("<img class='button' src='./images/del_button.png' " +
			"title='Delete current mapping' onclick='Controller.delMapping()'><br/>");
	mappings.push("</td><td style='text-align:center;vertical-align:center;'>");
	mappings.push("<ul id='menu_mappings'>");
	for (var i in list_mappings) {
		mappings.push("<li><span" + (i == current_mapping ? " class='active'" : "") + " title='Mapping_" + i
				+ "' onclick='displayMapping(" + i + ")'>Mapping " + i + "</span></li>");
	}
	mappings.push("</ul>");
	mappings.push("</td></tr>");

	mappings.push("<tr><td id='mapping_config' colspan='2'>");
	//mappings.push("<div id='mapping_config'>");
	for (var i in list_mappings) {
		mappings.push("<table id='Mapping_" + i + "' class='config-mapping' " +
				"style='display:" + (i == current_mapping ? "block" : "none") + ";text-align:center'>");
		mappings.push("<thead><th>Type</th><th>Parameters</th><th>Actions</th></thead>");
		mappings.push("<tbody id='Selectors_" + i + "'>");
		
		if (Object.keys(hh_mappings[list_mappings[i]]).length > 0) {
			var list_selectors = Object.keys(hh_mappings[list_mappings[i]].selectors);
			for (var j in list_selectors) {
				var current_selector = hh_mappings[list_mappings[i]].selectors[list_selectors[j]];
				mappings.push(View.createSelectorView[current_selector.type](list_selectors[j], current_selector.value, i, j));
			}
		}
		
		// Create an empty selector
		mappings.push(View.createEmptySelector(i));
		
		mappings.push("</tbody>");
		mappings.push("</table>");
		
		mappings.push(View.createMappingProfilesView(hh_mappings[list_mappings[i]], i, current_mapping));
	}
	
	
	//mappings.push("</div>");
	mappings.push("</td></tr>");
	mappings.push("</tbody></table>");
	
	return mappings.join('');
};

/*
 * Create an empty selector
 * \fn String View.createEmptySelector()
 * \param parent Integer Index of the mapping in which create a selector
 * \return String The HTML code
 */
View.createEmptySelector = function(parent) {
	var empty_selector = [];
	
	empty_selector.push("<tr>");
	empty_selector.push("<td><select id='new_selector_" + parent + "' onchange='Controller.addSelector(" + parent + ");'>");
	empty_selector.push("<option value='' selected></option><option value='env'>Environment</option></select></td>");
	empty_selector.push("<td></td><td></td>");
	empty_selector.push("</tr>");
	
	return empty_selector.join('');
}

/*
 * Create the available/selected profile section in a mapping.
 * \fn String View.createMappingProfilesViewmappings, nb, current_mapping)
 * \param mappings Object Hash table containing the data
 * \param nb Integer Index of the mapping
 * \param current_mapping Integer Current mapping displayed.
 * \return The HTML code.
 */
View.createMappingProfilesView = function(mappings, nb, current_mapping) {
	var available_profiles = Object.keys(myModel);
	var selected_profiles = mappings.profiles ? Object.keys(mappings.profiles) : [];
	var mapping_profiles = [];
	
	mapping_profiles.push("<table id='Mapping_profiles_" + nb + "' class='config-mapping-profiles'" +
			" style='display:" + (nb == current_mapping ? "block" : "none") + ";text-align:center'><thead>");
	mapping_profiles.push("<tr><th>Available profiles</th><th>Selected profiles</th></tr>");
	mapping_profiles.push("</thead><tbody><tr>");
	mapping_profiles.push("<td>");
	mapping_profiles.push("<select id='available_profiles_" + nb + "' multiple style='width:100px;height:100px;'>");
	
	for (var i in available_profiles) {
		if (available_profiles[i].value != "default" && selected_profiles.indexOf(available_profiles[i]) == -1) {
			mapping_profiles.push("<option value='" + available_profiles[i] + "'>" + available_profiles[i] + "</option>");
		}
	}
	
	mapping_profiles.push("</select>");
	mapping_profiles.push("</td><td>");
	mapping_profiles.push("<select id='selected_profiles_" + nb + "' multiple style='width:100px;height:100px;'>");
	
	for (var i in selected_profiles) {
		mapping_profiles.push("<option value='" + selected_profiles[i] + "'>" + selected_profiles[i] + "</option>")
	}
	
	mapping_profiles.push("</td>");
	mapping_profiles.push("</tr></tbody><tfoot><tr>");
	mapping_profiles.push("<td><input type='button' value='->' onclick='Controller.selectMappingProfile(" + nb + ")'/></td>");
	mapping_profiles.push("<td><input type='button' value='<-' onclick='Controller.unselectMappingProfile(" + nb + ")'/></td>");
	mapping_profiles.push("</tr></tfoot></table>");
	
	return mapping_profiles.join('');
};

//Hash table containing the association between a type and its function of view generation
View.createSelectorView = {
	"env": View.createEnvSelectorView,
};

/*
 * Update all the view
 * \fn void View.updateView(current_profile, current_mapping)
 * \param current_profile String Name of the displayed profile
 * \param current_mapping Integer Index of the displayed mapping
 */
View.updateView = function(current_profile, current_mapping) {
	View.updateProfilesView(current_profile);
	View.updateNetworksView();
	View.updateMappingsView(current_mapping);
	
	var new_config = Model.generateXmlConfig();
	document.getElementById('generated').innerHTML = 
		"<textarea id='generated_file' rows='15' cols='124' readonly></textarea><br/>" +
		"<input id='save' type='button' value='Save config file' onclick='saveXMLFile()'/>";
	document.getElementById('generated_file').value = new_config;
};

/*
 * Update the profiles view
 * \fn void View.updateProfilesView(current_profile)
 * \param current_profile String Name of the displayed profile
 */
View.updateProfilesView = function(current_profile) {
	var profile_view = View.createProfilesView(myModel, current_profile);
	
	document.getElementById('navigator').hidden = false;
	document.getElementById('menu_profiles').innerHTML = profile_view["menu_profiles"];
	document.getElementById('modules').innerHTML = profile_view["modules"];
	document.getElementById('current_profile').value = current_profile;
}

/*
 * Update the netwoks view.
 * \fn void View.updateNetworksView()
 */
View.updateNetworksView = function() {
	var networks_view = View.createNetworksView(myNetworks);
	document.getElementById('networks').innerHTML = networks_view;
}

/*
 * Update the mappings view
 * \fn void View.updateMappingsView(current_mapping)
 * \param current_mapping String Name of the displayed mapping
 */
View.updateMappingsView = function(current_mapping) {
	var mappings_view = View.createMappingsView(myMappings, current_mapping);
	document.getElementById('mappings').innerHTML = mappings_view;
}

/*
 * Update the XML.
 * \fn void View.updateXML()
 */
View.updateXML = function() {
	var new_config = Model.generateXmlConfig();
	document.getElementById('generated').innerHTML = "<textarea id='generated_file' rows='15' cols='124' readonly></textarea>"+
			"<input id='save' type='button' value='Save config file' onclick='saveXMLFile()'/>";
	document.getElementById('generated_file').value = new_config;
};

//Hash table containing the association between a type and its function of view generation
View.createView = {
		"param" : View.createParamView,
		"array" : View.createArrayField,
		"bool" : View.createCheckboxField,
		"int" : View.createTextField,
		"string" : View.createTextField,
		"funcptr" : View.createTextField,
		"size" : View.createSizeField
};

/*
 * Reset the content of a HTML element
 * \fn void View.clearElement()
 */
View.clearElement = function(element) {
	document.getElementById(element).innerHTML = "";
}

/*
 * Reset the view.
 * \fn void View.resetView()
 */
View.resetView = function() {
	var no_config = "<span style='font-style:italic;color:red;'>No loaded configuration file.</span>";
	document.getElementById('menu_profiles').innerHTML = no_config;
	document.getElementById('navigator').hidden = true;
	document.getElementById('modules').innerHTML = no_config;
	document.getElementById('networks').innerHTML = no_config;
	document.getElementById('mappings').innerHTML = no_config;
	document.getElementById('generated').innerHTML = no_config;
	
	list_cli_options = [];
	list_rails = [];
	list_drivers = [];
};
