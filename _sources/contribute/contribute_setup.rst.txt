===========================================
Prerequisites before starting to contribute
===========================================

General description
===================

This page should be read by anybody desiring to contribute to the MPC community and is intended, at first, for new users, helping them to set a proper environment before continuing. We encourage actual contributors to check their current compliance with these simple rules.

Git
---

To contribute, an proper ID has to be set, allowing other team members to identify contributors with clear, invariant names and emails. In order to do so, a best practice is to set your Git configuration file, done through the following command lines (`global` indicates to update your `~/.gitconfig` instead of local configuration, bound to the current project):

.. code-block:: bash

	git config --global user.email "youremail@domain.com"
	git config --global user.name "Firstname Lastname"


The two lines above are mandatory. Please note the following:

* Use the same email used by Gitlab (for proper contribution recognition)

* Check you do **NOT** have multiple identities in MPC history. To ensure that, run `git shortlog -sne` and look for duplicates. If so, please edit the `/.mailmap` file accordingly.

Not further restriction, but a lot of recommended setup, listed below, to ease your experience with MPC & Gitlab (and general-purpose Git usage):


.. code-block:: sh

	$ git config --global
	How to treat with "git push" (no arguments). We strongly recommend using this setup to avoid misuse!
	git config --global push.default simple
	# For kdiff3 aficionados when coming to resolve conflicts (vimdiff by default)
	git config --global merge.tool kdiff3
	#relative to the use of "git grep" command, a time-saving command for efficient programmers :)
	git config --global grep.extendRegexp true
	git config --global grep.lineNumber true
	git config --global grep.fallbackToNoIndex true
	# Memorize the way merge conflicts are resolved to re-apply it if the same conflict happens again
	git config --global rerere.enabled true
	# The only proof commits are from you... Anybody can take your identify otherwise !
	git config --global user.signingkey "First chars of your public GPG key"

There is plenty of nice features, we only list a few of them. If interested, feel free to contact <corentin.beaulieu@cea.fr> directly for more of his tips  & tricks around Git (like binary diff, compact git-status, tig-like without ncurses, tuned git-grep commands, powerful ways to dig into MPC history to identify when a function has been added/deleted...)

Gitlab
------

To access Gitlab from Git project, two approaches: HTTPS or SSH protocols. For the first one, you may need to set a proxy in your terminal (if your network is proxified before reaching Gitlab servers). Please look at `http[s]_proxy` environment variables. For SSH accesses, you will need to register an SSH key to you Gitlab profile (User profile (top-right) > Settings > SSH Keys) by copy-pasting your public key. A key pair can be generated as follows:

.. code-block:: sh

	# Follow the instruction (path & passphrase twice)
	ssh-keygen -t rsa
	# copy the content of this command into the Gitlab textbox
	cat ~/.ssh/id_rsa.pub

Through HTTPS protocol, you will have to type out a username/password each time Git accesses remote data (considering no password management tool), while SSH protocol will ask for your passphrase. A big difference here is that HTTPS protocol is subject to filtering/ban from our firewall, SSH is not.

MPC environment
---------------

To properly install and contribute to MPC, your building environment should have:

* A set of commands are assumed:`patch, tar, make, which`

* A proper compiler suite for C, C++, and Fortran

* Git installation (a recent version if possible)

* Optional: Spack (for MPC 4.0+ versions)

To be continued...
