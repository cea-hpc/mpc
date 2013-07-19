/*
 *
 */
var Controller = {};

/*
 * 
 */
Controller.confirmDelete = function(message) {
	var confirm = window.confirm(message);
	return confirm;
};

Controller.getCurrentMapping = function() {
	var menu_items = document.getElementById('menu_mappings').getElementsByTagName('span');
	for (var i = 0; i < menu_items.length; i ++) {
		if (menu_items[i].className == 'active') {
			return i;
		}
	}
	
	return -1;
}

/*
 * Create a new profile.
 * \fn void addProfile()
 */
Controller.addProfile = function() {
	var done = false;
	var canceled = false;
	var name = "";

	// Display prompt window until the user cancel the action,
	// or he gives a valid profile name
	do {
		name = window.prompt("Please enter the new profile name", "");

		// If the window prompt has been canceled
		if (name == null) {
			canceled = true;
			continue;
		}

		// If no profile name has been specified
		if (name == "") {
			alert("[Error] Please specify a profile name.. Or click cancel.");
			done = false;
			continue;
		}

		// If the profiling name matches to an existing profile
		if (myModel[name] != undefined) {
			alert("[Error] The profile name matches to an existing profile..");
			done = false;
			continue;
		}

		// The new profile name is correct
		done = true;
	}
	while (!done && !canceled);

	// If the action has been canceled
	if (canceled) {
		return;
	}

	// Associate the default values to the new profile
	myModel[name] = myModel["default"];

	// Update the view with the new data model
	View.updateView(name, Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
}

/*
 * Delete the current/selected profile.
 * \fn void delProfile()
 */
Controller.delProfile = function() {
	var message = "";
	
	// Get the name of the current profile
	var name = document.getElementById("current_profile").value;
	
	// Confirm the profile deletion
	message = "Are you sure that you want to delete the profile \"" + name + "\"?";
	var confirm = Controller.confirmDelete(message);
	if (!confirm) { // If the deletion has been canceled..
		message = "[Info] The deletion of the profile \"" + name + "\" has been canceled.";
		logMessage(message, false);
		return;
	}

	// The default profile cannot be renamed
	if (name == "default") {
		message = "[Error] The default profile cannot be deleted.";
		logMessage(message);
		return;
	}

	// Delete the current profile
	delete myModel[name];
	message = "[Info] The profile \"" + name + "\" has been deleted.";
	logMessage(message, false);

	// Update the view with the new data model
	View.updateView("default", Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
}

/*
 * Add a new mapping.
 * \fn void addMapping()
 */
Controller.addMapping = function() {
	// Add an empty mapping
	var idx = Object.keys(myMappings).length;
	myMappings[idx] = {};
	myMappings[idx].profiles = {};
	myMappings[idx].selectors = {};
	
	// Update the mappings view
	View.updateMappingsView(idx);
	
	// Activate this new mapping to display it
	displayMapping(idx);
	
	// Update the generated XML
	View.updateXML();
};

/*
 * Delete the current/selected mapping.
 * \fn void delMapping()
 */
Controller.delMapping = function() {
	var i = 0;
	var j = 0;
	
	// Confirm the mapping deletion
	message = "Are you sure that you want to delete the current mapping?";
	var confirm = Controller.confirmDelete(message);
	if (!confirm) { // If the deletion has been canceled..
		message = "[Info] The deletion of the current mapping has been canceled.";
		logMessage(message, false);
		return;
	}
	
	// Get the current/selected mapping
	var menu_mappings = document.getElementById("mappings").getElementsByTagName("span");
	for (i in menu_mappings) {
		if (menu_mappings[i].className == "active") {
			break;
		}
	}
	
	// Because there is no name associated to a mapping, this loop re-affects
	// the correct keys to the elements.
	for (j = parseInt(i); j < menu_mappings.length - 1; j ++) {
		myMappings[j] = myMappings[j + 1];
	}
	
	// Delete the last mapping element
	delete myMappings[j];
	
	// Update the mappings view and display the first mapping (0)
	View.updateMappingsView(0);
	displayMapping(0);
	
	// Update the generated XML
	View.updateXML();
};

Controller.updateSelectorName = function(parent, line_nb, element) {
	var selector_name = document.getElementById('selector_name_' + parent + '_' + line_nb).value;
	myMappings[parent].selectors[selector_name] = myMappings[parent].selectors[element.oldvalue];
	delete myMappings[parent].selectors[element.oldvalue];

	// Update the mappings view and display the first mapping (0)
	View.updateMappingsView(Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
}

Controller.updateSelectorValue = function(parent, line_nb, element) {
	var selector_name = document.getElementById('selector_name_' + parent + '_' + line_nb).value;
	myMappings[parent].selectors[selector_name].value = element.value;

	// Update the mappings view and display the first mapping (0)
	View.updateMappingsView(Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
}

/*
 * 
 */
Controller.addCliOption = function() {
	Controller.createNewCliOptions();
};

/*
 * 
 */
Controller.delCliOption = function() {
	var cli_options_span = document.getElementById("cli_options_vars").getElementsByTagName('span');
	var nb_cli_options = cli_options_span.length;
	var idx = -1;
	var cli_option_name = "";
	
	for (var i in cli_options_span) {
		if (cli_options_span[i].nodeType == 1 && cli_options_span[i].style.color == "red") {
			rail_name = myNetworks.networks.cli_options.values[i].name.value;
			idx = i;
			break;
		}
	}
	
	// Confirm the CLI option deletion
	message = "Are you sure that you want to delete the CLI option \"" + cli_option_name + "\"?";
	var confirm = Controller.confirmDelete(message);
	if (!confirm) { // If the deletion has been canceled..
		message = "[Info] The deletion of the CLI option \"" + cli_option_name + "\" has been canceled.";
		logMessage(message, false);
		return;
	}
	
	if (idx == -1) {
		var message = "[Warning] No CLI option selected.. Nothing to delete.";
		logMessage(message, true);
	}
	else {
		// Update the model
		Model.updateCliOptionsNetworksModel(cli_option_name, nb_cli_options, idx);
		
		// Update the view
		document.getElementById('cli_options_vars').innerHTML = View.updateCliOptionsView();		
		View.clearElement('network-print');
	}
};

/*
 * 
 */
Controller.addRail = function(cli_options_idx) {
	Controller.createNewRailCliOptions(cli_options_idx);
};

/*
 * 
 */
Controller.delRail = function() {
	var rails_span = document.getElementById("rails_vars").getElementsByTagName('span');
	var nb_rails = rails_span.length;
	var idx = -1;
	var rail_name = "";
	
	for (var i in rails_span) {
		if (rails_span[i].nodeType == 1 && rails_span[i].style.color == "red") {
			rail_name = myNetworks.networks.rails.values[i].name.value;
			idx = i;
			break;
		}
	}
	
	// Confirm the rail deletion
	message = "Are you sure that you want to delete the rail \"" + rail_name + "\"?";
	var confirm = Controller.confirmDelete(message);
	if (!confirm) { // If the deletion has been canceled..
		message = "[Info] The deletion of the rail \"" + rail_name + "\" has been canceled.";
		logMessage(message, false);
		return;
	}
	
	if (idx == -1) {
		var message = "[Warning] No rail selected.. Nothing to delete.";
		logMessage(message, true);
	}
	else {
		// Update the model
		Model.updateRailsNetworksModel(rail_name, nb_rails, idx);
		
		// Update the view
		document.getElementById('rails_vars').innerHTML = View.updateRailsView();		
		View.clearElement('network-print');
	}
};

/*
 * 
 */
Controller.addDriver = function(driver_idx) {
	Controller.createNewDriver(driver_idx);
};

/*
 * 
 */
Controller.delDriver = function() {
	var drivers_span = document.getElementById("drivers_vars").getElementsByTagName('span');
	var nb_drivers = drivers_span.length;
	var idx = -1;
	var driver_name = "";
	
	for (var i in drivers_span) {
		if (drivers_span[i].nodeType == 1 && drivers_span[i].style.color == "red") {
			driver_name = myNetworks.networks.configs.values[i].name.value;
			idx = i;
			break;
		}
	}
	
	// Confirm the driver deletion
	message = "Are you sure that you want to delete the driver \"" + driver_name + "\"?";
	var confirm = Controller.confirmDelete(message);
	if (!confirm) { // If the deletion has been canceled..
		message = "[Info] The deletion of the driver \"" + driver_name + "\" has been canceled.";
		logMessage(message, false);
		return;
	}
	
	if (idx == -1) {
		var message = "[Warning] No driver selected.. Nothing to delete.";
		logMessage(message, true);
	}
	else {
		// Update the model
		Model.updateDriversNetworksModel(driver_name, nb_drivers, idx);
		
		// Update the view
		document.getElementById('drivers_vars').innerHTML = View.updateDriversView();		
		View.clearElement('network-print');
	}
};

/*
 * Update the profile name.
 * \fn void update()
 */
Controller.renameProfile = function() {
	var console = document.getElementById("messages");
	var done = false;
	var canceled = false;
	var message = "";
	
	// Get the name of the current profile
	var old_name = document.getElementById("current_profile").value;

	// The default profile cannot be renamed
	if (old_name == "default") {
		message = "[Error] The default profile cannot be renamed.";
		logMessage(message);
		return;
	}

	do {
		var new_name = window.prompt("Please enter the new profile name", "");

		// If the window prompt has been canceled
		if (new_name == null) {
			canceled = true;
			message = "[Warning] The renaming of \"" + old_name + "\" has been canceled.";
			logMessage(message, false);
			continue;
		}

		// If no new name has been specified
		if (new_name == "") {
			done = false;
			message = "[Error] Please specify a new name.. Or click cancel.";
			logMessage(message);
			continue;
		}

		// If the new name is the same as the old one
		if (new_name == old_name) {
			done = false;
			message = "[Error] The old and the new names are equals..";
			logMessage(message);
			continue;
		}

		// If the new name matches to an existing profile
		if (myModel[new_name] != undefined) {
			done = false;
			message = "[Error] The new name matches to an existing profile..";
			logMessage(message);
			continue;
		}

		// The new profile name is correct
		done = true;
	}
	while (!done && !canceled);

	// If the action has been canceled
	if (canceled) {
		return;
	}

	// Update the model
	Model.renameProfile(old_name, new_name);

	// Update the profile name in the mappings
	Model.updateMappingsProfileName(old_name, new_name);
	
	// Update the view with the new data model
	View.updateView(new_name, Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
	
	// Log message
	var message = "[Info] Profile \"" + old_name + "\" has been renamed to \"" + new_name + "\".";
	console.value += message + "\n";
}

/*
 * Check if an element's value is an integer.
 * \fn void is_int(element)
 * \param Node element Element to check.
 */
Controller.is_int = function(element) {
	var profile = element.attributes["data-profile"].value;
	var module = element.attributes["data-module"].value;
	var property = element.attributes["data-property"].value;
	
	// If the element's value is not a valid integer
	if (!element.value.match("[0-9]+")) {
		var message = "[Error] \"" + property + "\" should be an integer value. " +
				"\"" + element.value + "\" is incorrect!";

		// Add a message error in the errors hash table
		addError(profile, module, property, message);
		
		// Log the message & alert
		logMessage(message);
	}
	else {
		// Delete the eventual error message associated to the given 
		// element in the errors hash table
		deleteError(profile, module, property);
	}
}

/*
 * Check if an element's value is a string.
 * \fn void is_string(element)
 * \param Node element Element to check.
 */
Controller.is_string = function(element) {
	var profile = element.attributes["data-profile"].value;
	var module = element.attributes["data-module"].value;
	var property = element.attributes["data-property"].value;
	
	// If the element's value is not a valid string
	// Only alpha-numerical characters and underscore are accepted
	if (!element.value.match("^[0-9A-Za-z_]+$")) {
		var message = "[Error] \"" + property + "\" should be a string value with " +
				"alpha numeric characters. \"" + element.value + "\" is incorrect!";
		
		// Add a message error in the errors hash table
		addError(profile, module, property, message);
		
		// Log the message & alert
		logMessage(message);
	}
	else {
		// Delete the eventual error message associated to the given 
		// element in the errors hash table
		deleteError(profile, module, property);
	}
}

/*
 * Check if an element's value is a function name.
 * \fn void is_funcptr(element)
 * \param Node element Element to check.
 */
Controller.is_funcptr = function(element) {
	var profile = element.attributes["data-profile"].value;
	var module = element.attributes["data-module"].value;
	var property = element.attributes["data-property"].value;
	
	// If the element's value is not a valid function name
	// A function name starts with letter or an underscore, and then contains alpha-numerical characters
	if (!element.value.match("^[A-Za-z_][0-9A-Za-z_]*$")) {
		var message = "[Error] \"" + property + "\" should be a function name. " +
				"\"" + element.value + "\" is incorrect!";
		
		// Add a message error in the errors hash table
		addError(profile, module, property, message);
		
		// Log the message & alert
		logMessage(message);
	}
	else {
		// Delete the eventual error message associated to the given 
		// element in the errors hash table
		deleteError(profile, module, property);
	}
}

/*
 * Check if an element's value is a size.
 * \fn void is_size(element)
 * \param Node element Element to check.
 */
Controller.is_size = function(element) {
	if (!element.value.match("[0-9]+[\s]?[K|M|G|T|P]?B")) {
		throw element + " should be a size value."
	}
}

/*
 * Check if an element's value is an enum.
 * \fn void is_enum(element)
 * \param Node element Element to check.
 */
Controller.is_enum = function(element) {
	var profile = element.attributes["data-profile"].value;
	var module = element.attributes["data-module"].value;
	var property = element.attributes["data-property"].value;
	
	var enum_values = meta.enum[element.type].values;
	var enum_keys = Object.keys(enum_values);
	var pattern = "[";
	
	// Compute the pattern with the possible enum values for the current element
	for (var i in enum_keys) {
		pattern += enum_values[enum_keys[i]] + ((i < enum_keys.length - 1) ? "|" : "");
	}
	
	// If the element's value is not a valid enum
	if (!element.value.match(pattern)) {
		var message = "[Error] \"" + property + "\" should be an enum value from \"" + element.type 
				+ "\". " + "\"" + element.value + "\" is incorrect!";

		// Add a message error in the errors hash table
		addError(profile, module, property, message);
		
		// Log the message & alert
		logMessage(message);
	}
	else {
		// Delete the eventual error message associated to the given 
		// element in the errors hash table
		deleteError(profile, module, property);
	}
}

/*
 * Update the profile when an element's value has been changed.
 * \fn void updateProfile(profile, module, property, element)
 * \param String profile Profile impacted by the change.
 * \param String module Module impacted by the change.
 * \param String property Property changed.
 * \param Node element HTML object associated to the property.
 */
Controller.updateProfile = function(profile, module, property, element) {
	// Update the model
	Model.updateModel(profile, module, property, element);
	
	// Update the generated XML
	View.updateXML();
};

/*
 * Update the profile when an element's value (array type) has been changed.
 * \fn void updateProfile(profile, module, property, element)
 * \param String profile Profile impacted by the change.
 * \param String module Module impacted by the change.
 * \param String property Property changed.
 * \param Node element HTML object associated to the property.
 */
Controller.updateArrayInProfile = function(profile, module, property, element) {
	// Update the model
	Model.updateArrayInModel(profile, module, property, element);
	
	// Update the generated XML
	View.updateXML();
};

/*
 * Update the profile when an element's value (size type) has been changed.
 * \fn void updateProfile(profile, module, property, element)
 * \param String profile Profile impacted by the change.
 * \param String module Module impacted by the change.
 * \param String property Property changed.
 */
Controller.updateSizeInProfile = function(profile, module, property) {
	// Update the model
	Model.updateSizeInModel(profile, module, property);
	
	// Update the generated XML
	View.updateXML();
};

/*
 * Add a profile in a mapping condition.
 * \fn void selectMappingProfile(mapping_index)
 * \param Integer mapping_index Index of the impacted mapping.
 */
Controller.selectMappingProfile = function(mapping_index) {
	var available_profiles = document.getElementById('available_profiles_'  + mapping_index);
	var length = available_profiles.length;

	for (var i = 0; i < length;) {
		var selected_profiles = document.getElementById('selected_profiles_'  + mapping_index);
		
		// If the current option is selected..
		if (available_profiles.options[i].selected) {
			// Get the selected option
			var profile_option =  available_profiles.options[i];
			
			// Add the profile in the "selected profiles" part
			selected_profiles.options[selected_profiles.length] = profile_option;
			
			// Delete the profile in the "available profiles" part
			length -= 1;
			
			// Update the mappings section in the model
			Model.updateMappingProfiles(profile_option.value, mapping_index, true);
		}
		// If the current option is not selected..
		else {
			i ++;
		}
	}

	// Update the generated XML
	View.updateXML();
};

/*
 * Remove a profile from a mapping condition.
 * \fn void unselectMappingProfile(mapping_index)
 * \param Integer mapping_index Index of the impacted mapping.
 */
Controller.unselectMappingProfile = function(mapping_index) {
	var selected_profiles = document.getElementById('selected_profiles_'  + mapping_index);
	var length = selected_profiles.length;

	for (var i = 0; i < length;) {
		var available_profiles = document.getElementById('available_profiles_'  + mapping_index);
		
		// If the current option is selected..
		if (selected_profiles.options[i].selected) {
			// Get the selected option
			var profile_option =  selected_profiles.options[i];
			
			// Add the profile in the "available profiles" part
			available_profiles.options[available_profiles.length] = profile_option;
			
			// Delete the profile in the "selected profiles" part
			length -= 1;
			
			// Update the mappings section in the model
			Model.updateMappingProfiles(profile_option.value, mapping_index, false);
		}
		// If the current option is not selected..
		else {
			i ++;
		}
	}

	// Update the generated XML
	View.updateXML();
};

Controller.createNewCliOptions = function() {
	var cli_option_name = "";
	var canceled = false;
	var done = false;
	
	do {
		cli_option_name = window.prompt("Please enter the new cli_option name", "");

		// If the window prompt has been canceled
		if (cli_option_name == null) {
			canceled = true;
			message = "[Warning] The creation of a new cli_option has been canceled.";
			logMessage(message, false);
			continue;
		}

		// If no rail name has been specified
		if (cli_option_name == "") {
			done = false;
			message = "[Error] Please specify a cli_option name.. Or click cancel.";
			logMessage(message);
			continue;
		}

		// If the new name matches to an existing profile
		if (list_cli_options.indexOf(cli_option_name) != -1) {
			done = false;
			message = "[Error] The cli_option name matches to an existing cli_option..";
			logMessage(message);
			continue;
		}

		// The new rail name is correct
		done = true;
	}
	while (!done && !canceled);

	// If the action has been canceled
	if (canceled) {
		return;
	}
	
	//
	var idx = Model.createNewCliOption(cli_option_name);
	
	//
	list_cli_options.push(cli_option_name);
	
	//
	var tbody = document.getElementById("cli_options_vars");
	var row_count = tbody.rows.length;
	var row = tbody.insertRow(row_count);
	var cell = row.insertCell(0);
	
	var span_element = document.createElement('span');
	var text = document.createTextNode(cli_option_name);
	span_element.className = 'network-th';
	span_element.onclick = function onclick(event) { View.printConfig(idx, 'network-print', this) };
	span_element.appendChild(text);
	cell.appendChild(span_element);
	
	//
	View.printConfig(idx, "network-print", null);
	
	// Update the generated XML
	View.updateXML();
};

Controller.createNewRailCliOptions = function(cli_options_idx, in_cli_options) {
	var rail_name = "";
	var canceled = false;
	var done = false;
	
	do {
		rail_name = window.prompt("Please enter the new rail name", "");

		// If the window prompt has been canceled
		if (rail_name == null) {
			canceled = true;
			message = "[Warning] The creation of a new rail has been canceled.";
			logMessage(message, false);
			continue;
		}

		// If no rail name has been specified
		if (rail_name == "") {
			done = false;
			message = "[Error] Please specify a rail name.. Or click cancel.";
			logMessage(message);
			continue;
		}

		// If the new name matches to an existing profile
		if (list_rails.indexOf(rail_name) != -1) {
			done = false;
			message = "[Error] The rail name matches to an existing rail..";
			logMessage(message);
			continue;
		}

		// The new rail name is correct
		done = true;
	}
	while (!done && !canceled);

	// If the action has been canceled
	if (canceled) {
		return;
	}
	
	//
	//Model.updateMappingProfiles(value, rail_index, true);
	//Model.updateRailsCliOptions(cli_options_idx, i, true);
	var idx = Model.createNewRail(rail_name);
	
	//
	list_rails.push(rail_name);
	
	//
	in_cli_options = in_cli_options ? in_cli_options : false;
	if (in_cli_options == true) {
		var selected_rails = document.getElementById("selected_rails_" + cli_options_idx);
		selected_rails.options[selected_rails.length] = new Option(rail_name, rail_name);
	}
	
	//
	var tbody = document.getElementById("rails_vars");
	var row_count = tbody.rows.length;
	var row = tbody.insertRow(row_count);
	var cell = row.insertCell(0);
	
	var span_element = document.createElement('span');
	var text = document.createTextNode(rail_name);
	span_element.className = 'network-th';
	span_element.onclick = function onclick(event) { View.printRail(idx, 'network-print', this) };
	span_element.appendChild(text);
	cell.appendChild(span_element);
	
	//
	var nb_rails = document.getElementById("rails_vars").getElementsByTagName("tr").length;
	View.printRail(idx, (in_cli_options ? "add-rail" : "network-print"), null);
	
	// Update the generated XML
	View.updateXML();
};

Controller.createNewDriver = function(driver_idx, in_rail) {
	var driver_name = "";
	var canceled = false;
	var done = false;
	
	do {
		driver_name = window.prompt("Please enter the new driver name", "");

		// If the window prompt has been canceled
		if (driver_name == null) {
			canceled = true;
			message = "[Warning] The creation of a new driver has been canceled.";
			logMessage(message, false);
			continue;
		}

		// If no driver name has been specified
		if (driver_name == "") {
			done = false;
			message = "[Error] Please specify a driver name.. Or click cancel.";
			logMessage(message);
			continue;
		}

		// If the new name matches to an existing driver
		if (list_drivers.indexOf(driver_name) != -1) {
			done = false;
			message = "[Error] The driver name matches to an existing driver..";
			logMessage(message);
			continue;
		}

		// The new rail name is correct
		done = true;
	}
	while (!done && !canceled);

	// If the action has been canceled
	if (canceled) {
		return;
	}
	
	//
	var idx = Model.createNewDriver(driver_name);
	
	//
	list_drivers.push(driver_name);
	
	//
	in_rail = in_rail ? in_rail : false;
	if (in_rail == true) {
		var choices = document.getElementById('driver_selection');
		var new_choice = new Option(driver_name, driver_name);
		new_choice.selected = "selected";
		choices.options[choices.options.length - 1] = new_choice;
		choices.options[choices.options.length] = new Option('undefined', 'undefined');
	}
	
	//
	var tbody = document.getElementById("drivers_vars");
	var row_count = tbody.rows.length;
	var row = tbody.insertRow(row_count);
	var cell = row.insertCell(0);
	
	var span_element = document.createElement('span');
	var text = document.createTextNode(driver_name);
	span_element.className = 'network-th';
	span_element.onclick = function onclick(event) { View.printDriver(idx, 'network-print', this) };
	span_element.appendChild(text);
	cell.appendChild(span_element);
	
	//
	var nb_rails = document.getElementById("rails_vars").getElementsByTagName("tr").length;
	View.printDriver(idx, (in_rail ? "add-driver" : "network-print"), null);
	
	// Update the generated XML
	View.updateXML();
};

/*
 * Add a rail in a CLI options configuration from a give profile.
 * \fn void selectRailCliOptions(profile_name, cli_options_idx)
 * \param String profile_name Impacted profile.
 * \param Integer cli_options_idx Index of the impacted CLI options in the profile.
 */
Controller.selectRailCliOptions = function(profile_name, cli_options_idx) {
	var available_rails = document.getElementById('available_rails_'  + cli_options_idx);
	var length = available_rails.length;

	// Run over all the options of the "available rails" part to work on the selected ones
	for (var i = 0; i < length;) {
		var selected_rails = document.getElementById('selected_rails_'  + cli_options_idx);
		
		// If the current option is selected..
		if (available_rails.options[i].selected) {
			// Get the selected option
			var rail_option = available_rails.options[i];
			
			// Add the rail in the "selected rails" part
			selected_rails.options[selected_rails.length] = rail_option;
			
			// The number of available rails is decrement by one
			length -= 1;
			
			// Update the model
			Model.updateRailsCliOptions(profile_name, cli_options_idx, i, true);
		}
		// If the current option is not selected
		else {
			i ++;
		}
	}

	// Update the generated XML
	View.updateXML();
};

/*
 * Remove a rail in a CLI options configuration from a give profile.
 * \fn void unselectRailCliOptions(profile_name, cli_options_idx)
 * \param String profile_name Impacted profile.
 * \param Integer cli_options_idx Index of the impacted CLI options in the profile.
 */
Controller.unselectRailCliOptions = function(profile_name, cli_options_idx) {
	var selected_rails = document.getElementById('selected_rails_'  + cli_options_idx);
	var length = selected_rails.length;

	// Run over all the options of the "selected rails" part to work on the selected ones
	for (var i = 0; i < length;) {
		var available_rails = document.getElementById('available_rails_'  + cli_options_idx);
		
		// If the current option is selected..
		if (selected_rails.options[i].selected) {
			// Get the selected option
			var rail_option = selected_rails.options[i];
			
			// Add the rail in the "available rails" part
			available_rails.options[available_rails.length] = rail_option;

			// The number of selected rails is decrement by one
			length -= 1;
			
			// Update the model
			Model.updateRailsCliOptions(profile_name, cli_options_idx, i, false);
		}
		// If the current option is not selected..
		else {
			i ++;
		}
	}

	// Update the generated XML
	View.updateXML();
};

Controller.addSelector = function(parent) {
	// Get the type of the new selector
	var select = document.getElementById('new_selector_' + parent)
	var type = select.options[select.selectedIndex].value;
	
	// Only do something when the type is not null
	if (type != "") {
		// Get the name of the selector
		var name = "";
		while (name == "" && name != null) {
			name = window.prompt("Enter the name of the new selector :", "");
		}
		
		// If the name is null, creation has been canceled
		if (name == null) {
			return;
		}

		// Update the model
		Model.updateMappingSelectors(name, type, "", parent, true);

		//
		View.updateMappingsView(parent);

		// Update the generated XML
		View.updateXML();
	}
};

Controller.delSelector = function(parent, line_nb) {
	var mapping = document.getElementById("Mapping_" + parent).getElementsByTagName("tbody")[0];
	var selector = XML.getChildNodes(mapping)[line_nb];
	var childs_selector = XML.getChildNodes(selector);
	
	// Get the name of the selector
	var id_name = "selector_name_" + parent + "_" + line_nb;
	var name = document.getElementById(id_name).value;
	
	// Confirm the selector deletion
	message = "Are you sure that you want to delete the selector \"" + name + "\"?";
	var confirm = Controller.confirmDelete(message);
	if (!confirm) { // If the deletion has been canceled..
		message = "[Info] The deletion of the selector \"" + name + "\" has been canceled.";
		logMessage(message, false);
		return;
	}
	
	// Get the type of the selector
	var id_type = "selector_type_" + parent + "_" + line_nb;
	var idx = document.getElementById(id_type).selectedIndex;
	var type = document.getElementById(id_type).options[idx].value;
	
	// Get the value of the selector
	var id_value = "selector_value_" + parent + "_" + line_nb;
	var value = document.getElementById(id_value).value;
	
	mapping.removeChild(selector);
	alert("Mapping: " + parent + ", line: " + line_nb);
	
	// Update the model
	Model.updateMappingSelectors(name, type, value, parent, false);
	
	// Update the generated XML
	View.updateXML();
};