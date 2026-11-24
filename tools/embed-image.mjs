import {readFileSync, writeFileSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
const [source, output, object = 'surface-exercise', mode = 'surface', manifest] = process.argv.slice(2);
if (!source || !output || !['surface', 'cli', 'manager'].includes(mode)) { throw new Error('embed-image SOURCE OUTPUT OBJECT surface|cli|manager'); }
function load(path) {
    const bytes=readFileSync(path);
    if (bytes.length>65536 || bytes.includes(0)) throw new Error('Bundled images must be NUL-free and at most 64 KiB');
    return new TextDecoder('utf-8',{fatal:true}).decode(bytes);
}
const images=source==='-' ? [] : [{name:'image.neo',text:load(source)}];
if (manifest) {
    const description=JSON.parse(readFileSync(manifest,'utf8'));
    if (description.format!=='neo.bundle/1' || !Array.isArray(description.images)) throw new Error('Expected neo.bundle/1');
    for (const image of description.images) {
        if (!image || typeof image.name!=='string' || !/^[a-zA-Z0-9_-][a-zA-Z0-9_.-]{0,58}\.neo$/.test(image.name) ||
            typeof image.source!=='string' || !image.source || images.some(prior=>prior.name===image.name) || images.length===8) throw new Error('Invalid or duplicate bundled image');
        images.push({name:image.name,text:load(resolve(dirname(manifest),image.source))});
    }
}
const text=images[0]?.text ?? '';
writeFileSync(output, `/* Generated from the canonical source; do not edit. */\n` +
    `static const char neo_bundle_source[] = ${JSON.stringify(text).replace(/\\u0000/g, '\\0')};\n` +
    `static const char neo_bundle_object[] = ${JSON.stringify(object)};\n` +
    `static const char neo_bundle_mode[] = ${JSON.stringify(mode)};\n` +
    `static const struct { const char *name,*source; } neo_bundle_images[] = {\n`+
    (images.length ? images.map(image=>`{${JSON.stringify(image.name)},${JSON.stringify(image.text)}}`).join(',\n') : '{0,0}')+`\n};\n`+
    `static const size_t neo_bundle_count = ${images.length};\n`);
