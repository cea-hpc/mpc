# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'MPC'
copyright = '2025, CEA'
author = 'CEA'
release = '4.3'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ['sphinx_rtd_theme', 'breathe', 'myst_parser']

templates_path = ['_templates']
exclude_patterns = []

breathe_projects = {'MPC': '../build/doxygen/xml'}
breathe_default_project = 'MPC'
breathe_default_members = ("members", "undoc-members")
breathe_domain_by_extension = {"h": "c", "c": "c"}
breathe_show_define_initializer = True
breathe_show_enumvalue_initializer = True
breathe_show_include = False



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_logo = "_static/mpc_logo.png"
html_static_path = ['_static']

html_theme_options = {
    'logo_only': True,
    'display_version': False,
}
