
import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.absolute()))
from overpass_query import query_city
import argparse
import logging
import time
from constants import *
import pos_convert

from common.db import SessionLocal
from common.model import QueryTask

def update_or_create_task(city, radius, result_path: pathlib.Path):
    session = SessionLocal()

    task = session.query(QueryTask).filter(
        QueryTask.city == city,
        QueryTask.radius == radius
    ).first()

    if task is None:
        task = QueryTask(
            city=city,
            radius=radius,
            status="finished",
            result_path=result_path.as_posix()
        )
        session.add(task)
    else:
        task.status = "finished"
        task.result_path = result_path.as_posix()

    session.commit()
    session.close()

def query_poi_list(city_name: str, radius: int = 25000, log: bool = False):
    query_city.query_cities_poi(city_name, radius, log)
    pos_convert.process_poi(city_name)
    pos_convert.poi_pos_convert(city_name, 'json')

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Query city data from OpenStreetMap')
    parser.add_argument('-c','--city_name', type=str, help='City name',required=True)
    parser.add_argument('-r','--radius', type=int, default=25000, help='Radius of the query in meters')
    parser.add_argument('--poi', action='store_true', default=False, help='Query POI data instead of roads')
    parser.add_argument('--log', action='store_true', default=False, help='Log the output')

    args = parser.parse_args()

    start_time = time.time()
    if args.poi:
        query_poi_list(args.city_name, args.radius, args.log)
    else:
        query_city.query_city(args.city_name, args.radius, args.log)
        update_or_create_task(args.city_name, args.radius, DATA_DIR/f"city/{args.city_name}/city.gpkg")
    print(f"Querying city {args.city_name} took {time.time() - start_time:.2f} seconds")