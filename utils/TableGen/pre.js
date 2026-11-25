var Module = {
    preRun: [
        function () {
            const fs = require('fs');
            const path = require('path');

            const source =
                '@CPPINTEROP_SOURCE_DIR@';

            const destination = '/lib/CppInterOp';

            function copyDirectory(src, dest) {
                FS.mkdirTree(dest);

                for (const entry of fs.readdirSync(src, {
                    withFileTypes: true
                })) {
                    const srcPath = path.join(src, entry.name);
                    const destPath = dest + '/' + entry.name;

                    if (entry.isDirectory()) {
                        copyDirectory(srcPath, destPath);
                    } else {
                        FS.writeFile(
                            destPath,
                            fs.readFileSync(srcPath)
                        );
                    }
                }
            }

            copyDirectory(source, destination);
        }
    ]
};
