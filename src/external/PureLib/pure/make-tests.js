// make-tests.js - Create test files automatically.

var Node = {
  fs: require('fs'),
  path: require('path'),
  zlib: require('zlib')
};
var assert = require('assert');

function set(options, key, value) {
  assert(typeof key === 'string');
  assert(value !== undefined);
  assert(Number.isInteger(value) || Buffer.isBuffer(value));
  if (!options.hasOwnProperty(key)) options[key] = value;
}

function EOCDR(options = {}) {
  set(options, 'signature', Buffer.from('504b0506', 'hex'));
  set(options, 'disk', 0);
  set(options, 'cd_disk', options.disk);
  set(options, 'cd_records', 0);
  set(options, 'cd_disk_records', options.cd_records);
  set(options, 'cd_size', 0);
  set(options, 'cd_offset', 0);
  set(options, 'comment', Buffer.alloc(0));
  set(options, 'comment_length', options.comment.length);
  var buffer = Buffer.alloc(22 + options.comment.length);
  var offset = 0;
  offset += options.signature.copy(buffer, offset);
  offset = buffer.writeUInt16LE(options.disk, offset);
  offset = buffer.writeUInt16LE(options.cd_disk, offset);
  offset = buffer.writeUInt16LE(options.cd_disk_records, offset);
  offset = buffer.writeUInt16LE(options.cd_records, offset);
  offset = buffer.writeUInt32LE(options.cd_size, offset);
  offset = buffer.writeUInt32LE(options.cd_offset, offset);
  offset = buffer.writeUInt16LE(options.comment_length, offset);
  offset += options.comment.copy(buffer, offset);
  assert(offset === buffer.length);
  return buffer;
}

if (!Node.fs.existsSync('tests')) Node.fs.mkdirSync('tests');

[
  [
    'PURE_E_OK',
    EOCDR()
  ],
  [
    'PURE_E_ZIP_TOO_SMALL',
    EOCDR().slice(0, 21)
  ],
  [
    'PURE_E_ZIP_RAR',
    Buffer.from('526172211a0700000000000000000000000000000000', 'hex')
  ],
  [
    'PURE_E_ZIP_TAR',
    Buffer.from('75737461720000000000000000000000000000000000', 'hex')
  ],
  [
    'PURE_E_ZIP_XAR',
    Buffer.from('78617221000000000000000000000000000000000000', 'hex')
  ],
  [
    'PURE_E_ZIP_EOCDR_NOT_FOUND',
    EOCDR({ signature: Buffer.from('504b0505', 'hex') })
  ],
  [
    'PURE_E_ZIP_EOCDR_SIZE_OVERFLOW',
    EOCDR({ cd_records: 2, cd_size: 46 * 2 - 1 })
  ],
  [
    'PURE_E_ZIP_EOCDR_SIZE_UNDERFLOW',
    EOCDR({ cd_records: 0, cd_size: 46 })
  ],
  [
    'PURE_E_ZIP_CD_EOCDR_OVERFLOW',
    EOCDR({ cd_offset: 0, cd_records: 1, cd_size: 46 })
  ],
  [
    'PURE_E_ZIP_EOCDR_RECORDS',
    EOCDR({ cd_disk_records: 1, cd_records: 0 })
  ],
  [
    'PURE_E_ZIP_MULTIPLE_DISKS',
    EOCDR({ cd_disk: 1 })
  ],
  [
    'PURE_E_ZIP_APPENDED_DATA_ZEROED',
    Buffer.concat([EOCDR(), Buffer.alloc(13, 0)])
  ],
  [
    'PURE_E_ZIP_APPENDED_DATA_BUFFER_BLEED',
    Buffer.concat([EOCDR(), Buffer.alloc(97, 1)])
  ],
  [
    'PURE_E_ZIP_PREPENDED_DATA',
    Buffer.concat([Buffer.alloc(2048, 1), EOCDR()])
  ],
  [
    'PURE_E_ZIP_PREPENDED_DATA_BUFFER_BLEED',
    Buffer.concat([Buffer.alloc(1, 1), EOCDR()])
  ],
  [
    'PURE_E_ZIP_PREPENDED_DATA_ZEROED',
    Buffer.concat([Buffer.alloc(1, 0), EOCDR()])
  ]
].forEach(
  function(test, index) {
    var code = test[0];
    var buffer = test[1];
    var name = index.toString().padStart(3, '0') + '_' + code + '.zip';
    var path = Node.path.join('tests', name);
    console.log('Creating ./' + path);
    Node.fs.writeFileSync(path, buffer);
  }
);

