const fs = require("node:fs");
const vm = require("node:vm");
const assert = require("node:assert/strict");
const path = require("node:path");
const file = path.join(__dirname, "../reports/city_world_map.html");
const html = fs.readFileSync(file, "utf8");
assert(!html.includes("__CITY_DATA__") && !html.includes("__WORLD_PATHS__"));
assert((html.match(/<path d=/g) || []).length > 100);
assert.equal((html.match(/<circle /g) || []).length, 269);
assert(!html.includes("__STATIC_POINTS__") && !html.includes("__SUMMARY__"));

class Element {
  constructor() {
    this.children = []; this.style = {}; this.events = {}; this.attributes = {};
    this.value = ""; this.textContent = ""; this.hidden = false;
    this.classList = { add() {}, remove() {}, toggle() {} };
  }
  setAttribute(key, value) { this.attributes[key] = value; }
  appendChild(child) {
    if (child.parent) child.parent.children = child.parent.children.filter(c => c !== child);
    child.parent = this; this.children.push(child); return child;
  }
  append(...children) { children.forEach(child => this.appendChild(child)); }
  replaceChildren() { this.children = []; }
  addEventListener(name, callback) { this.events[name] = callback; }
}
const ids = new Map([...html.matchAll(/id="([^"]+)"/g)].map(match => [match[1], new Element()]));
const document = {
  getElementById(id) { assert(ids.has(id), "Missing element: " + id); return ids.get(id); },
  createElement() { return new Element(); },
  createElementNS() { return new Element(); }
};
const context = vm.createContext({ document, console });
vm.runInContext(html.match(/<script>([\s\S]*?)<\/script>/)[1], context);
const run = code => vm.runInContext(code, context);
assert.equal(run("circles.length"), 269);
assert.equal(ids.get("city-list").children.length, 18);
assert.equal(run("buttons.size"), 269);
assert.equal(run("cityGroups.size"), 269);
assert.equal(run("[...new Set([...cityGroups.values()].map(g => g.state))].length"), 104);
assert.equal(run("cities.every((c,i) => {const g=cityGroups.get(i);return g.country.children[0].children[0].textContent===c.country && g.state.children[0].children[0].textContent===c.state;})"), true);
assert.equal(ids.get("startup-status").hidden, true);
assert.equal(ids.get("static-points").style.display, "none");
run("circles[1].events.click()");
assert.equal(ids.get("city-name").textContent, "阿姆斯特丹");
assert.equal(ids.get("state-name").textContent, "北荷兰省");
assert.equal(ids.get("country-name").textContent, "荷兰");
assert.equal(run("cityGroups.get(1).country.open && cityGroups.get(1).state.open"), true);
ids.get("country").value = "美国"; ids.get("country").events.change();
assert.equal(ids.get("city-list").children.length, 1);
assert.equal(run("buttons.size"), 33);
assert.equal(run("circles.filter(c => c.style.display !== 'none').length"), 33);
ids.get("country").value = ""; ids.get("search").value = "泰州"; ids.get("search").events.input();
assert.equal(ids.get("city-list").children.length, 1);
run("[...buttons.values()][0].events.click()");
assert.equal(ids.get("state-name").textContent, "江苏省");
assert.equal(run("zoom"), 5);
ids.get("search").value = "not-a-city"; ids.get("search").events.input();
assert.equal(run("visible.length"), 0);
assert.equal(ids.get("details").hidden, true);
ids.get("search").value = ""; ids.get("search").events.input();
ids.get("reset").events.click();
assert.equal(run("zoom"), 1);
ids.get("zoom-in").events.click();
assert.equal(run("zoom"), 1.5);
console.log("PASS: 269 cities in 18 countries / 104 regions; grouping, selection, filters and zoom.");
