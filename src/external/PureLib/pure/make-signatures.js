// make-signatures.js - Create file format signature constants.

var Node = {
  fs: require('fs')
};

var signatures = {
  ICONR:           [0x49, 0x63, 0x6f, 0x6e, 0x0d],
  RAR:             [0x52, 0x61, 0x72, 0x21, 0x1a, 0x07],
  TAR:             [0x75, 0x73, 0x74, 0x61, 0x72],
  XAR:             [0x78, 0x61, 0x72, 0x21],
  ZIP_CDH:         [0x50, 0x4b, 0x01, 0x02],
  ZIP_DDR:         [0x50, 0x4b, 0x07, 0x08],
  ZIP_EOCDL_64:    [0x50, 0x4b, 0x06, 0x07],
  ZIP_EOCDR_64:    [0x50, 0x4b, 0x06, 0x06],
  ZIP_EOCDR:       [0x50, 0x4b, 0x05, 0x06],
  ZIP_LFH:         [0x50, 0x4b, 0x03, 0x04],
  ZIP_PK:          [0x50, 0x4b],
  ZIP_SPAN:        [0x50, 0x4b, 0x07, 0x08],
  ZIP_TEMP:        [0x50, 0x4b, 0x30, 0x30]
};

var lines = [];

function line(string) {
  lines.push(string);
}

function arrayName(name, octets) {
  return 'const uint8_t PURE_S_' + name + '[' + octets.length + ']';
}

function hex(octets) {
  return octets.map(
    function(octet) {
      return '0x' + octet.toString(16).padStart(2, '0');
    }
  ).join(', ');
}

var max = 0;
for (var name in signatures) {
  var octets = signatures[name];
  var length = arrayName(name, octets).length;
  if (length > max) max = length;
}
for (var name in signatures) {
  var octets = signatures[name];
  var padded = arrayName(name, octets).padEnd(max, ' ');
  line(padded + ' = {' + hex(octets) + '};');
}
line('');
for (var name in signatures) {
  var octets = signatures[name];
  var padded = ('const uint64_t PURE_L_' + name).padEnd(max, ' ');
  line(padded + ' = ' + octets.length + ';');
}

Node.fs.writeFileSync('pure_signatures.h', lines.join('\n'), 'utf-8');
