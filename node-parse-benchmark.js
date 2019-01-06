const max = 1000000;

const sources = new Array(max);

for (let i = 0; i < max; i++) {
  sources[i] = "{\"array\":[null,true,false,-1.0e-2,\"string\"," + Math.random() + "]}";
}

const t0 = Date.now();

for (let i = 0; i < max; i++) {
  JSON.parse(sources[i]);
}

const t1 = Date.now();

console.log(`${t1 - t0}ms`);
