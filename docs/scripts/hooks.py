"""properdocs hooks: regenerate the cvar reference before every build."""

import importlib.util
import logging
from pathlib import Path

SCRIPTS_DIR = Path(__file__).resolve().parent

log = logging.getLogger("properdocs.hooks.voxelius")


def load_script(name):
    spec = importlib.util.spec_from_file_location(name.replace("-", "_"), SCRIPTS_DIR / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def on_pre_build(config, **kwargs):
    cvars = load_script("make-cvars")
    pages, _, _ = cvars.generate()

    for name, text in pages.items():
        if cvars.write_if_changed(cvars.DEFAULT_DIR / name, text):
            log.info(f"regenerated {name}")
