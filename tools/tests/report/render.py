"""
    Functions to generate the HTML output
"""
from jinja2 import Template
import os

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

def render_result(testCount, testcaseCount, passed, skiped, failed, outputDir):
    test_results = []

    summary_stats = {
        'total': testCount,
        'individual': testcaseCount,
        'pass': len(passed),
        'skip': len(skiped),
        'fail': len(failed)
    }

    for item in passed:
        test_results.append({
            'name': item,
            'result': 'pass',
        })

    for item in skiped:
        test_results.append({
            'name': item,
            'result': 'skip',
        })

    failed_dir = os.path.join(outputDir, "fail")
    for item in failed:
        test_basename = item.replace("..", "_").replace("./", "").replace("/", "_")

        with open(os.path.join(failed_dir, test_basename + ".out"), 'r') as f:
            output = f.read()
        with open(os.path.join(failed_dir, test_basename + ".exp"), 'r') as f:
            expect = f.read()

        test_results.append({
            'name': item,
            'result': 'fail',
            'expect': expect,
            'output': output
        })

    sorted_test_results = sorted(test_results, key=lambda x: x['name'])

    context = {
        'test_report_title': 'CanMV Test Report',
        'test_summary': summary_stats,
        'test_results': sorted_test_results,
    }

    template = load_template(os.path.join(outputDir, "../report/template.html"))
    rendered_template = render_template(template, context)
    with open(os.path.join(outputDir, "report.html"), 'w') as template_file:
        template_file.write(rendered_template)
