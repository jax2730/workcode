"""Compile and exercise the actual city registration methods without Android builds."""

import argparse
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("actor", type=Path)
    parser.add_argument("activity", type=Path)
    parser.add_argument("catalog", type=Path)
    parser.add_argument("--javac", type=Path, required=True)
    args = parser.parse_args()

    actor = json.loads(args.actor.read_text(encoding="utf-8-sig"))
    original = next(
        component["cityList"]
        for component in actor["ComponentArray"]
        if component["Factory"] == "KarstCityRegistryComponent"
    )
    catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
    assert len(original) == len(catalog) == 269
    for source, exported in zip(original, catalog):
        assert source["name"] == exported["name_en"]
        assert source["pos"] == exported["pos"]
        assert all(exported[key] for key in ("name", "country", "state"))

    source = args.activity.read_text(encoding="utf-8-sig")
    assert source.count("        initRegisteredCities();") == 1
    start = source.index("    private void initLocationData()")
    end = source.index("    private void initLocationSelectionUI()", start)
    methods = source[start:end]
    harness = """
import java.util.*;
public class RegisteredCityCheck {
    Map<String, Map<String, String[]>> locationData = new HashMap<>();
    Map<String, String> coordinatesData = new HashMap<>();
""" + methods + """
    public static void main(String[] args) throws Exception {
        System.setOut(new java.io.PrintStream(System.out, true, "UTF-8"));
        RegisteredCityCheck check = new RegisteredCityCheck();
        Map<String, String[]> existing = new HashMap<>();
        existing.put("北京市", new String[]{"北京市", "legacy-city"});
        check.locationData.put("中国", existing);
        check.coordinatesData.put("legacy-city", "1,200,2");
        check.coordinatesData.put("北京市", "old-coordinate");
        check.initLocationData();
        check.initLocationData();
        if (check.coordinatesData.size() != 269 || check.coordinatesData.containsKey("legacy-city")) {
            throw new AssertionError("Legacy coordinates retained or city coordinates missing");
        }
        for (Map.Entry<String, Map<String, String[]>> country : check.locationData.entrySet()) {
            for (Map.Entry<String, String[]> state : country.getValue().entrySet()) {
                for (String city : state.getValue()) {
                    System.out.println(country.getKey() + "\\t" + state.getKey()
                        + "\\t" + city + "\\t" + check.coordinatesData.get(city));
                }
            }
        }
    }
}
"""
    with tempfile.TemporaryDirectory(prefix="city-registration-") as directory:
        java_source = Path(directory) / "RegisteredCityCheck.java"
        java_source.write_text(harness, encoding="utf-8")
        subprocess.run(
            [str(args.javac), "-encoding", "UTF-8", "-d", directory, str(java_source)],
            check=True, capture_output=True, text=True,
        )
        result = subprocess.run(
            [str(args.javac.with_name("java.exe")), "-Dfile.encoding=UTF-8",
             "-cp", directory, "RegisteredCityCheck"],
            check=True, capture_output=True, text=True, encoding="utf-8",
        )

    records = [line.split("\t") for line in result.stdout.splitlines()]
    assert len(records) == len(catalog), "Legacy, duplicate or missing menu entries"
    registered = {tuple(record[:3]): record[3] for record in records}
    special = {
        "Binhai": "滨海新区", "Fuling": "涪陵区", "Wanzhou": "万州区",
        "Guangfeng": "广丰区", "Hong Kong": "香港", "Xiong'an": "雄安新区",
    }
    for city in catalog:
        label = city["name"]
        if city["country"] == "中国":
            label = special.get(city["name_en"], label + "市")
        coordinates = registered.pop((city["country"], city["state"], label))
        x, y, z = map(float, coordinates.split(","))
        assert [x * 2, z * 2] == city["pos"], city["name_en"]
        assert y == 1500
    assert not registered
    print("PASS: exactly 269 actor cities; all regions and positions match; legacy entries removed; no duplicates.")


if __name__ == "__main__":
    main()
