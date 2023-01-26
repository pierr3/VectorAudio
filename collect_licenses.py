import os
import glob
from pathlib import Path

vcpkg_packages_root = os.path.join('vcpkg', 'packages')

vcpkg_packages_array = [ f.path for f in os.scandir(vcpkg_packages_root) if f.is_dir() ]

master_license = ''
output_file = os.path.join('resources', 'LICENSE.txt')

# Adding the base licenses

master_license += 'Vector Audio\n\n' + Path('LICENSE').read_text() + '\n\n'

# Adding the licenses of submodules

submodules_licenses = [['afv-native', os.path.join('extern', 'afv-native', 'COPYING.md')], 
    ['ImGui', os.path.join('extern', 'imgui', 'LICENSE.txt')], 
    ['Platform Folders', os.path.join('extern', 'PlatformFolders', 'LICENSE')],
    ['Airports Database', os.path.join('resources', 'LICENSE_AIRPORTS')]]

for sublicense in submodules_licenses:
    print('Getting license for: ' + sublicense[0])
    master_license += sublicense[0] + '\n\n' + Path(sublicense[1]).read_text() + '\n\n'

# Adding dependencies licenses

for package in vcpkg_packages_array:
    package_name = package.split('/')[-1]
    print('Getting license for: ' + package_name, end=' ')

    licenses = glob.glob(os.path.join(package, '**', 'copyright'), recursive = True)
    if (len(licenses) >= 1):
        print('Found %i licenses' % len(licenses))
        master_license += package_name + '\n\n'

        for license in licenses:
            master_license += Path(license).read_text() + '\n\n'
    else:
        print('No license found!')


f = open(output_file, "w")
f.writelines(master_license)
f.close()