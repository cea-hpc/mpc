# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import subprocess
from datetime import date

current_year = date.today().year

project = "MPC"
author = "Commissariat à l'Énergie Atomique et aux Énergies Alternatives"
copyright = str(current_year) + ", " + author
version = subprocess.run(["../../utils/get_version"], text=True, capture_output=True).stdout
release = version

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["sphinx_rtd_theme", "breathe", "myst_parser"]

templates_path = ["_templates"]
exclude_patterns = []

breathe_projects = {"MPC": "../build/doxygen/xml"}
breathe_default_project = "MPC"
breathe_default_members = ("members", "undoc-members")
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_show_define_initializer = True
breathe_show_enumvalue_initializer = True
breathe_show_include = False

pygments_style = "sphinx"

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_logo = "_static/mpc_logo.png"
html_favicon = "_static/mpc_logo.png"
html_static_path = ["_static"]

html_theme_options = {
    "logo_only": True,
}
