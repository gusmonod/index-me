'use strict';

let childProcess = require('child_process');
let co = require('co');
let bluebird = require('bluebird');
let fs = require('fs');
let path = require('path');
let stream = require('stream');

bluebird.promisifyAll(fs);
let exec = bluebird.promisify(childProcess.exec);

const kConcurrency = 5;
const kMaxBuffer = 10 << 20;  // 10 MB
const kCorpusPath = process.env.CORPUS_PATH ||
                    path.resolve(__dirname, '../../corpus');

module.exports = co(function*() {
  let corpusFiles = yield fs.readdirAsync(kCorpusPath);
  let zFiles = corpusFiles.filter(name => /\.z$/.test(name));
  let documents = yield bluebird.map(zFiles,
    filepath => new Promise((resolve, reject) => {
      const cmd = `bash -c zcat < "${path.join(kCorpusPath, filepath)}"`;
      exec(cmd, {maxBuffer: kMaxBuffer})
          .then(result => resolve(result[0]))
          .catch(e => reject(e));
    }),
    {concurrency: kConcurrency}
  );
  let cleanDocs = documents.map(doc => doc.toLowerCase()
    // Removes markup tags
    .replace(/(<\!--.*-->)|(<\s*\/?\s*[a-z-]+(\s+[a-z-]+\s*=\s*\"(\\\"|[^\"])*\")*\s*\/?\s*>)/g, '')
    // Replaces non letters with a space
    .replace(/[^a-z]+/g, ' ')
    // Removes duplicate whitespaces
    .replace(/\s+/g, ' ')
    // Removes trailing and leading spaces
    .replace(/^ /g, '')
    .replace(/ $/g, '')
  );

  return cleanDocs;
});
