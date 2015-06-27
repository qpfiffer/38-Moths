from ctypes import cdll

class Context(object):
    def __init__(self, context_dict):
        pass

class Template(object):
    def __init__(self, template):
        self.template = template
        self.greshunkel_lib = cdll.load("lib38moths.so")

    def render(context):
        # Hand-wavy rendering right here.
        pass
