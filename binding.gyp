{
  'variables': {
    'gpp': '<!(type g++ | grep "is" > /dev/null && echo "yep" || echo "no")',
    'node_version': '<!(node --version | cut -c 4,5)'
  },
  'targets': [{
    'target_name': 'sockit',
    'sources': ['src/node-0.<@(node_version)/addon.c',
                'src/node-0.<@(node_version)/sockit.cc'],
    'conditions': [[
      '"<@(gpp)"=="no"',
      { 'sources!': ['src/node-0.<@(node_version)/addon.c',
                     'src/node-0.<@(node_version)/sockit.cc'] }
    ]]
  }]
}
