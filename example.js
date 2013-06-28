// Import the add-on.
var addon = require('./build/Release/sockit');

// Create a new sockit object.
var sockit = new addon.Sockit();

// Connect to a host and port.
sockit.connect({ host: "www.google.com", port: 80 });

// Write some data!
sockit.write("GET / HTTP/1.1\n\n");

// Read some tasty bytes!
var data = sockit.read(1024);

// Observe amazing results.
console.log("Read ", data.length, " bytes");

// And beautiful raw HTTP response.
console.log(data);

// Close it up.
sockit.close();

