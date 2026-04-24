import subprocess
import os

def build():
    c_dir = 'effects_c'
    build_dir = 'build'
    output = 'effects/cext.dll'
    
    os.makedirs(build_dir, exist_ok=True)
    
    EXCLUDE = {'main.c'}
    sources = [
        os.path.join(c_dir, f) for f in os.listdir(c_dir) if f.endswith('.c') and f not in EXCLUDE
    ]
    
    cmd = [
        'cl', '/LD', '/O2', '/wd5045',
        f'/Fo{build_dir}\\',
        *sources,
        f'/Fe:{output}',
        '/link', '/DLL', f'/DEF:{c_dir}/exports.def', '/MACHINE:X64'
    ]
    
    print(f"building {output}")
    try:
        subprocess.run(cmd, check=True)
        print("success")
    except Exception as e:
        print(f"build failed")

if __name__ == "__main__":
    build()