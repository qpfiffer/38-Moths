from ctypes import cdll, c_char_p, c_size_t

lib38moths = cdll.LoadLibrary("lib38moths.so")

def _add_item_to_greshunkel_loop(loop, value):
    if isinstance(value, str):
        lib38moths.gshkl_add_string_to_loop(loop, c_char_p(value.encode()))
    elif isinstance(value, int):
        lib38moths.gshkl_add_int_to_loop(ctext, c_size_t(value))
    elif isinstance(value, list):
        raise Exception("Cannot add loops to loops right now. Use subcontexts.")
    elif isinstance(value, dict):
        raise NotImplementedError()

def _add_item_to_greshunkel_context(ctext, key, value):
    if isinstance(value, str):
        lib38moths.gshkl_add_string(ctext, c_char_p(key.encode()), c_char_p(value.encode()))
    elif isinstance(value, int):
        lib38moths.gshkl_add_int(ctext, c_char_p(key.encode()), c_size_t(value))
    elif isinstance(value, list):
        new_loop = lib38moths.gshkl_add_array(ctext, c_char_p(key.encode()))
        for subitem in value:
            _add_item_to_greshunkel_loop(new_loop, subitem)
    elif isinstance(value, dict):
        raise NotImplementedError()

class Context(object):
    def __init__(self, context_dict):
        self.gshkl_ctext = lib38moths.gshkl_init_context()
        for key, value in context_dict.items():
            _add_item_to_greshunkel_context(self.gshkl_ctext, key, value)

class Template(object):
    def __init__(self, template):
        self.template = template

    def render(self, context):
        template_c_string = c_char_p(self.template.encode())
        ret_c_str = lib38moths.gshkl_render(context.gshkl_ctext, template_c_string, len(self.template), None)
        return ret_c_str.decode()
