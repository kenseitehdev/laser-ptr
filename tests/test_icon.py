import pathlib
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
probe=str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as d:
    frames={}
    for active in (0,1):
        for light in (0,1):
            path=pathlib.Path(d)/f'{active}-{light}.svg'
            subprocess.run([probe,'icon',str(path),str(active),str(light)],check=True)
            root=ET.parse(path).getroot()
            assert root.attrib['viewBox']=='0 0 28 22'
            frames[active,light]=path.read_text()
            assert ('#ffffff' if light else '#000000') in frames[active,light]
            assert ('fill-opacity' in frames[active,light]) == bool(active)
            assert 'scale(-1 -1)' in frames[active,light]
    assert frames[0,0] != frames[1,0]
print('Four SVG icon variants passed XML, state, outline and left-facing checks.')
