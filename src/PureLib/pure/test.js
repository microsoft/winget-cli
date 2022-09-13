var Node = {
  fs: require('fs'),
  path: require('path')
};
var Pure = require('./binding.node');
var assert = require('assert');

var files = Node.fs.readdirSync('tests');
for (var index = 0; index < files.length; index++) {
  var file = files[index];
  if (!/zip$/i.test(file)) console.log('SKIPPING: ' + file);
  var buffer = Node.fs.readFileSync(Node.path.join('tests', file));
  var flags = 0;
  var code = file.replace(/\.zip$/, '').replace(/^\d{3}_/, '');
  var result = Pure.zip(buffer, flags);
  if (result.code === code) {
    console.log('PASS: ' + file + ': ' + result.message);
  } else {
    console.log('');
    console.error('FAIL: ' + file + ': ' + result.code + ': ' + result.message);
    process.exit(1);
  }
}
