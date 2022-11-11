"""
    Functions to generate the HTML output
"""
from jinja2 import Template


def load_template(template_file_path):
    """
    Load a Jinja2 template and return it

    Args:
        template_file_path (str): path to the Jinja2 template file

    Returns:
        Template: contains jinja2 template
    """
    return Template(open(template_file_path).read())


def render_template(template, context):
    """
    Generate an HTML test report.

    Args:
        template (Template): Jinja2 Template object containing the template to render
        context (dict): the context to pass to the template

    Returns:
        str: the contents of the rendered template
    """
    return template.render(context)