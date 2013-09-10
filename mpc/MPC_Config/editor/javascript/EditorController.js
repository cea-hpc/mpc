/*!
 * \file EditorController.js
 * \namespace Controller
 * Functions helpers to synchronize model and view.
 */
var Controller = {};

/*!
 * Open a popup to confirm or not a deletion action
 * \fn Boolean Controller.confirmDelete(message)
 * \param message String Message to display in the confirm box.
 * \return Boolean false to stop the deletion, true otherwise.
 */
Controller.confirmDelete = function(message) {
	var confirm = window.confirm(message);
	return confirm;
};

/*!
 * Get the index of the current displayed mapping.
 * \fn Integer Controller.getCurrentMapping()
 * \return Integer The index of the displayed mapping, -1 if there is a problem.
 */
Controller.getCurrentMapping = function() {
	var menu_items = document.getElementById('menu_mappings').getElementsByTagName('span');
	for (var i = 0; i < menu_items.length; i ++) {
		if (menu_items[i].className == 'active') {
			return i;
		}
	}
	
	return -1;
}

/*!
 * Create a new profile.
 * \fn void Controller.addProfile()
 */
Controller.addProfile = function() {
	var done = false;
	var canceled = false;
	var name = "";
	var message = "";

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
			message = "[Error] Please specify a profile name.. Or click cancel.";
			logMessage(message);
			done = false;
			continue;
		}

		// If the profiling name matches to an existing profile
		if (myModel[name] != undefined) {
			message = "[Error] The profile name matches to an existing profile..";
			logMessage(message);
			done = false;
			continue;
		}

		// The new profile name is correct
		done = true;
	}
	while (!done && !canceled);

	// If the action has been canceled
	if (canceled) {
		message = "[Warning] The creation of a new profile has been canceled.";
		logMessage(message, false);
		return;
	}

	// Associate the default values to the new profile
	myModel[name] = clone(myModel["default"]);

	// Update the view with the new data model
	View.updateView(name, Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
}

/*!
 * Delete the current/selected profile.
 * \fn void Controller.delProfile()
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

/*!
 * Add a new mapping.
 * \fn void Controller.addMapping()
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

/*!
 * Delete the current/selected mapping.
 * \fn void Controller.delMapping()
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
	message = "[Info] The current mapping has been deleted.";
	logMessage(message, false);
	
	// Update the mappings view and display the first mapping (0)
	View.updateMappingsView(0);
	displayMapping(0);
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Update the name of a selector.
 * \fn void Controller.updateSelectorName(parent, line_nb, element)
 * \param parent Integer Index of the mapping containing this selector.
 * \param line_nb Integer Index of this selector in the mapping.
 * \param element Element Input field containing the selector name.
 */
Controller.updateSelectorName = function(parent, line_nb, element) {
	var selector_name = document.getElementById('selector_name_' + parent + '_' + line_nb).value;
	myMappings[parent].selectors[selector_name] = myMappings[parent].selectors[element.oldvalue];
	delete myMappings[parent].selectors[element.oldvalue];

	// Update the mappings view and display the first mapping (0)
	View.updateMappingsView(Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
}

/*!
 * Update the value of a selector.
 * \fn void Controller.updateSelectorValue(parent, line_nb, element)
 * \param parent Integer Index of the mapping containing this selector.
 * \param line_nb Integer Index of this selector in the mapping.
 * \param element Element Input field containing the selector value.
 */
Controller.updateSelectorValue = function(parent, line_nb, element) {
	var selector_name = document.getElementById('selector_name_' + parent + '_' + line_nb).value;
	myMappings[parent].selectors[selector_name].value = element.value;

	// Update the mappings view and display the first mapping (0)
	View.updateMappingsView(Controller.getCurrentMapping());
	
	// Update the generated XML
	View.updateXML();
}

/*!
 * Add a new CLI options
 * \fn void Controller.addCliOption()
 */
Controller.addCliOption = function() {
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
	
	// Update the model with this new CLI options
	var idx = Model.createNewCliOption(cli_option_name);
	
	// Add the new CLI options to the list of existing ones
	list_cli_options.push(cli_option_name);
	
	// Update the CLI options view
	document.getElementById("cli_options_vars").innerHTML = View.updateCliOptionsView();
	
	// Print the properties of this new CLI options
	View.printCliOption(idx, "network-print", null);
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Delete the selected CLI options
 * \fn void Controller.delCliOptions()
 */
Controller.delCliOption = function() {
	var cli_options_span = document.getElementById("cli_options_vars").getElementsByTagName('span');
	var nb_cli_options = cli_options_span.length;
	var idx = -1;
	var cli_option_name = "";
	
	// Get the index of the selected CLI options
	for (var i in cli_options_span) {
		if (cli_options_span[i].nodeType == 1 && cli_options_span[i].style.color == "red") {
			cli_option_name = myNetworks.networks.cli_options.values[i].name.value;
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
	
	// If no CLI options selected..
	if (idx == -1) {
		var message = "[Warning] No CLI option selected.. Nothing to delete.";
		logMessage(message, true);
	}
	// Otherwise
	else {
		// Update the model
		Model.updateCliOptionsNetworksModel(cli_option_name, nb_cli_options, idx);
		message = "[Info] The CLI option \"" + cli_option_name + "\" has been deleted.";
		logMessage(message, true);
		
		// Update the view
		document.getElementById('cli_options_vars').innerHTML = View.updateCliOptionsView();		
		View.clearElement('network-print');
	}
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Add a new rail.
 * \fn void Controller.addRail(cli_options_idx)
 * \param cli_option_idx Integer [Optional] Index of the CLI option in which add the rail.
 */
Controller.addRail = function(cli_options_idx) {
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
	
	// Update the model with this new rail
	var idx = Model.createNewRail(rail_name);
	
	// Add the new rail to the list of existing ones
	list_rails.push(rail_name);
	
	// If the rail has been directly added into a CLI option
	if (cli_options_idx != undefined) {
		var selected_rails = document.getElementById("selected_rails_" + cli_options_idx);
		selected_rails.options[selected_rails.length] = new Option(rail_name, rail_name);
		
		Model.updateRailsCliOptions(cli_options_idx, idx, true);
	}
	
	// Update the view
	document.getElementById('rails_vars').innerHTML = View.updateRailsView();
	
	// Print the properties of this new rail
	View.printRail(idx, (cli_options_idx != undefined ? "add-rail" : "network-print"), null);
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Delete the selected rail.
 * \fn void Controller.delRail()
 */
Controller.delRail = function() {
	var rails_span = document.getElementById("rails_vars").getElementsByTagName('span');
	var nb_rails = rails_span.length;
	var idx = -1;
	var rail_name = "";
	
	// Get the index of the selected rail
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
	
	// If no rail selected..
	if (idx == -1) {
		var message = "[Warning] No rail selected.. Nothing to delete.";
		logMessage(message, true);
	}
	// Otherwise..
	else {
		// Update the model
		Model.updateRailsNetworksModel(rail_name, nb_rails, idx);
		message = "[Info] The rail \"" + rail_name + "\" has been deleted.";
		logMessage(message, true);
		
		// Update the view
		document.getElementById('rails_vars').innerHTML = View.updateRailsView();		
		View.clearElement('network-print');
	}
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Add a new driver.
 * \fn void Controller.addDriver(driver_idx)
 * \param driver_idx Integer [Optional] Index of the rail in which add the driver.
 */
Controller.addDriver = function(driver_idx) {
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
	
	// Update the model with this new driver
	var idx = Model.createNewDriver(driver_name);
	
	// Add the new driver to the list of existing ones
	list_drivers.push(driver_name);
	
	// If the driver has been directly added into a rail
	if (driver_idx != undefined) {
		var choices = document.getElementById('driver_selection');
		var new_choice = new Option(driver_name, driver_name);
		new_choice.selected = "selected";
		choices.options[choices.options.length - 1] = new_choice;
		choices.options[choices.options.length] = new Option('(null)', 'undefined');
	}
	
	// Update the view
	document.getElementById('drivers_vars').innerHTML = View.updateDriversView();
	
	// Print the properties of this new driver
	View.printDriver(idx, (driver_idx ? "add-driver" : "network-print"), null);
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Delete the selected driver
 */
Controller.delDriver = function() {
	var drivers_span = document.getElementById("drivers_vars").getElementsByTagName('span');
	var nb_drivers = drivers_span.length;
	var idx = -1;
	var driver_name = "";
	
	
	// Get the index of the selected driver
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
	
	// If no driver selected..
	if (idx == -1) {
		var message = "[Warning] No driver selected.. Nothing to delete.";
		logMessage(message, true);
	}
	// Otherwise
	else {
		// Update the model
		Model.updateDriversNetworksModel(driver_name, nb_drivers, idx);
		message = "[Info] The driver \"" + driver_name + "\" has been deleted.";
		logMessage(message, true);
		
		// Update the view
		document.getElementById('drivers_vars').innerHTML = View.updateDriversView();		
		View.clearElement('network-print');
	}
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Update the profile name.
 * \fn void Controller.update()
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
	logMessage(message, false);
}

/*!
 * Update the profile when an element's value has been changed.
 * \fn void Controller.updateProfile(profile, module, property, element)
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

/*!
 * Update the profile when an element's value (array type) has been changed.
 * \fn void Controller.updateProfile(profile, module, property, element)
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

/*!
 * Update the profile when an element's value (size type) has been changed.
 * \fn void Controller.updateProfile(profile, module, property, element)
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

/*!
 * Update the network when an element's value has been changed.
 * \fn void Controller.updateNetwork()
 */
Controller.updateNetwork = function(section, index, property, element) {
	// Update the network
	Model.updateNetwork(section, index, property, element);
	
	// Update the generated XML
	View.updateXML();
};

/*!
 * Add a profile in a mapping condition.
 * \fn void Controller.selectMappingProfile(mapping_index)
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

/*!
 * Remove a profile from a mapping condition.
 * \fn void Controller.unselectMappingProfile(mapping_index)
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

/*!
 * Add a rail in a CLI options configuration from a give profile.
 * \fn void Controller.selectRailCliOptions(profile_name, cli_options_idx)
 * \param String profile_name Impacted profile.
 * \param Integer cli_options_idx Index of the impacted CLI options in the profile.
 */
Controller.selectRailCliOptions = function(cli_options_idx) {
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
			Model.updateRailsCliOptions(cli_options_idx, i, true);
		}
		// If the current option is not selected
		else {
			i ++;
		}
	}

	// Update the generated XML
	View.updateXML();
};

/*!
 * Remove a rail in a CLI options configuration from a give profile.
 * \fn void Controller.unselectRailCliOptions(profile_name, cli_options_idx)
 * \param String profile_name Impacted profile.
 * \param Integer cli_options_idx Index of the impacted CLI options in the profile.
 */
Controller.unselectRailCliOptions = function(cli_options_idx) {
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
			Model.updateRailsCliOptions(cli_options_idx, i, false);
		}
		// If the current option is not selected..
		else {
			i ++;
		}
	}

	// Update the generated XML
	View.updateXML();
};

/*!
 * Add a new selector in the current mapping.
 * \fn void Controller.addSelector(parent)
 * \param parent Integer Index of the current mapping.
 */
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

		// Update the mapping view
		View.updateMappingsView(parent);

		// Update the generated XML
		View.updateXML();
	}
};

/*!
 * Delete a selector in the current mapping.
 * \fn void Controller.delSelector(parent, line_nb)
 * \param parent Integer Index of the current mapping.
 * \param line_nb Integer Index of the selector to delete in the mapping.
 */
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
		
	// Update the model
	Model.updateMappingSelectors(name, type, value, parent, false);
	message = "[Info] The selector \"" + name + "\" has been deleted.";
	logMessage(message, true);

	// Update the mapping view
	View.updateMappingsView(parent);
	
	// Update the generated XML
	View.updateXML();
};