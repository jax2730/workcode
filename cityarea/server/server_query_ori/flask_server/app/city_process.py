from flask import Flask, request, jsonify, Blueprint, render_template, send_file
import subprocess
import sys
import pathlib
sys.path.append(str(pathlib.Path(__file__).parent.parent.parent.absolute()))
from common.db import SessionLocal
from common.model import QueryTask, SimplyfyTask, LandmarkTask, StreatBlockTask
from constants import *
import os
import json
import re
import datetime

bp = Blueprint('city_process', __name__,template_folder="/home/pixel/geo_tools/flask_server/templates")

@bp.route('/index', methods=['GET'])
def index():
    return render_template('city_query.html')

@bp.route('/city_query', methods=['POST'])
def city_query():
    data = request.get_json()
    city = data['city']
    if 'radius' in data:
        radius = data['radius']
    else:
        radius = 25000
    query_dict = {'city': city, 'radius': radius}
    # TODO：create query id
    session = SessionLocal()
    task = session.query(QueryTask).filter_by(city=city, radius=radius).first()
    if task:
        task_id = task.id
        session.close()
        return jsonify({'task_id': task_id})
    task = QueryTask(city=city, radius=radius, status='pending')
    session.add(task)
    session.commit()
    task_id = task.id
    session.close()
    subprocess.Popen([
        'python', '/home/pixel/geo_tools/city_process/query_city.py', 
        '-c',city, 
        '-r',str(radius),
        '--log'
    ],
    stdout=open(DATA_DIR/'query.log', 'a'),
    stderr=open(DATA_DIR/'query.err', 'a')
    )
    return jsonify({'task_id': task_id})

@bp.route('/city_poi', methods=['POST'])
def city_poi():
    data = request.get_json()
    city = data['city']
    if 'radius' in data:
        radius = data['radius']
    else:
        radius = 25000
    query_dict = {'city': city, 'radius': radius}
    subprocess.run([
        'python','/home/pixel/geo_tools/city_process/query_city.py', 
        '-c', city,
        '-r', str(radius),
        '--poi'
    ],
    stdout=open(DATA_DIR/'query.log', 'a'),
    stderr=open(DATA_DIR/'query.err', 'a')
    )
    return send_file(
        DATA_DIR / 'city'/ city/ 'poi_pos.json',
        as_attachment=True,
        download_name=f'{city}_poi.json'
    )

@bp.route('/city_query/<task_id>', methods=['GET'])
def get_city_query_status(task_id):
    session = SessionLocal()
    task = session.query(QueryTask).filter(QueryTask.id == task_id).first()
    if task:
        status = task.status
        if status == 'finished':
            result_path = task.result_path
            session.close()
            return jsonify({'status': status,'result_path': result_path})
        else:
            session.close()
            return jsonify({'status': status})
    else:
        session.close()
        return jsonify({'status': 'not found'})
    
@bp.route("/download_query/<int:task_id>", methods=["GET"])
def download_query(task_id):
    session = SessionLocal()
    task = session.query(QueryTask).filter(QueryTask.id == task_id).first()
    session.close()

    if not task:
        return jsonify({"error": "task not found"}), 404

    if task.status != "finished":
        return jsonify({"error": "task not finished"}), 400

    if not task.result_path or not os.path.exists(task.result_path):
        return jsonify({"error": "result file not found"}), 404

    city = task.city
    return send_file(
        task.result_path,
        as_attachment=True,
        download_name=f'{city}_{pathlib.Path(task.result_path).name}'
    )

@bp.route('/city_simplify', methods=['POST'])
def city_simplify():
    data = request.get_json()
    city = data['city']
    if 'tolerance' in data:
        tolerance = data['tolerance']
    else:
        tolerance = 50
    session = SessionLocal()
    queryTask = session.query(QueryTask).filter_by(city=city).first()
    if not queryTask:
        session.close()
        return jsonify({'error': f'{city} not found in query results, Please query city data first.'})
    simplifyTask = session.query(SimplyfyTask).filter_by(city=city, tolerance=tolerance).first()
    if simplifyTask:
        task_id = simplifyTask.id
        session.close()
        return jsonify({'task_id': task_id})
    simplifyTask = SimplyfyTask(city=city, tolerance=tolerance, status='pending')
    session.add(simplifyTask)
    session.commit()
    task_id = simplifyTask.id
    session.close()
    subprocess.Popen([
        'python', '/home/pixel/geo_tools/city_process/simplify_road.py', 
        '-s', '1',
        '-t',str(tolerance),
        city
    ],
    stdout=open(DATA_DIR/'simplify.log', 'a'),
    stderr=open(DATA_DIR/'simplify.err', 'a')
    )
    return jsonify({'task_id': task_id})

@bp.route('/city_simplify/<task_id>', methods=['GET'])
def get_city_simplify_status(task_id):
    session = SessionLocal()
    task = session.query(SimplyfyTask).filter(SimplyfyTask.id == task_id).first()
    if task:
        status = task.status
        if status == 'finished':
            result_path = task.result_path
            session.close()
            return jsonify({'status': status,'result_path': result_path})
        else:
            session.close()
            return jsonify({'status': status})
    else:
        session.close()
        return jsonify({'status': 'not found'})

@bp.route("/download_simplify/<int:task_id>", methods=["GET"])
def download_simplify(task_id):
    session = SessionLocal()
    task = session.query(SimplyfyTask).filter(SimplyfyTask.id == task_id).first()
    session.close()

    if not task:
        return jsonify({"error": "task not found"}), 404

    if task.status != "finished":
        return jsonify({"error": "task not finished"}), 400

    if not task.result_path or not os.path.exists(task.result_path):
        return jsonify({"error": "result file not found"}), 404

    city = task.city
    return send_file(
        DATA_DIR / 'city'/ city / 'city_road_w_degree1.gpkg',
        task.result_path,
        as_attachment=True,
        download_name=f'{city}_road_w_degree1.gpkg'
    )

@bp.route('/query_landmark', methods=['POST'])
def query_landmark():
    data = request.get_json()
    city = data['city']
    session = SessionLocal()
    queryTask = session.query(QueryTask).filter_by(city=city).first()
    if not queryTask:
        session.close()
        return jsonify({'error': f'{city} not found in query results, Please query city data first.'})
    session.close()
    landmarks = data['landmarks']
    landmark_basic = DATA_DIR / 'city'/ city / 'landmarks.json'
    with open(landmark_basic, 'w') as f:
        f.write(json.dumps(landmarks))
    
    subprocess.run([
        'python', '/home/pixel/geo_tools/city_process/query_landmark.py', 
        '-c', city,
        '-f', str(landmark_basic)
    ],
        stdout=open(DATA_DIR/'query.log', 'a'),
        stderr=open(DATA_DIR/'query.err', 'a')
    )
    
    return send_file(
        DATA_DIR / 'city'/ city/ 'landmarks' / 'landmarks_poly.gpkg',
        as_attachment=True,
        download_name=f'{city}_landmarks_poly.gpkg'
    )

@bp.route('/upload_landmark', methods=['POST'])
def upload_landmark():
    city = request.form.get('city')
    session = SessionLocal()
    simplyfyTask = session.query(SimplyfyTask).filter_by(city=city).first()
    if not simplyfyTask:
        session.close()
        return jsonify({'error': f'{city} not found in simplify results, Please simplify city data first.'})
    file = request.files.get('file')
    if not file:
        return jsonify({'error': 'file not found','hint': 'If no landmark in the city, please upload an empty gpkg file.'})
    file_path = DATA_DIR / 'city'/ city / 'landmark_polygons.gpkg'
    if file_path.exists():
        file_path.unlink()
    file.save(file_path)
    complete_status = subprocess.run([
        'python', '/home/pixel/geo_tools/city_process/simplify_road.py', 
        '-s', '2',
        city
    ],
    stdout=open(DATA_DIR/'simplify.log', 'a'),
    stderr=open(DATA_DIR/'simplify.err', 'a')
    )
    return send_file(
        DATA_DIR / 'city'/ city / 'city_road_w_landmark_degree2.gpkg',
        as_attachment=True,
        download_name=f'{city}_road_w_landmark_degree2.gpkg'
    )

@bp.route('/street_block', methods=['POST'])
def create_street_block():
    city = request.form.get('city')
    print(city)
    session = SessionLocal()
    landmarkTask = session.query(LandmarkTask).filter_by(city=city).first()
    if not landmarkTask:
        session.close()
        return jsonify({'error': f'{city} not found in landmark results, Please query landmark data first.'})
    street_block_task = session.query(StreatBlockTask).filter_by(city=city).first()
    if street_block_task:
        create_time: datetime.datetime = street_block_task.created_at
        current_time = datetime.datetime.now()
        if (current_time - create_time).seconds < 60*60:
            task_id = street_block_task.id
            session.close()
            return jsonify({'task_id': task_id})
    file = request.files.get('file')
    if file:
        file_path = DATA_DIR / 'city'/ city / 'city_road_w_landmark_degree2.gpkg'
        if file_path.exists():
            file_path.unlink()
        file.save(file_path)
    if not street_block_task:
        street_block_task = StreatBlockTask(city=city, status='pending')
        session.add(street_block_task)
    else:
        street_block_task.status = 'pending'
        street_block_task.created_at = datetime.datetime.now()
    session.commit()
    task_id = street_block_task.id
    session.close()
    subprocess.Popen([
        'python', '/home/pixel/geo_tools/city_process/street_block_process.py', 
        city
    ],
    stdout=open(DATA_DIR/'street_block.log', 'a'),
    stderr=open(DATA_DIR/'street_block.err', 'a')
    )
    return jsonify({'task_id': task_id})

@bp.route('/street_block/<task_id>', methods=['GET'])
def get_street_block_status(task_id):
    session = SessionLocal()
    task = session.query(StreatBlockTask).filter(StreatBlockTask.id == task_id).first()
    if task:
        status = task.status
        if status == 'finished':
            result_path = task.result_path
            session.close()
            return jsonify({'status': status,'result_path': result_path})
        else:
            session.close()
            return jsonify({'status': status})
    else:
        session.close()
        return jsonify({'status': 'not found'})

@bp.route("/download_street_block/<int:task_id>", methods=["GET"])
def download_street_block(task_id):
    session = SessionLocal()
    task = session.query(StreatBlockTask).filter(StreatBlockTask.id == task_id).first()
    session.close()

    if not task:
        return jsonify({"error": "task not found"}), 404

    if task.status != "finished":
        return jsonify({"error": "task not finished"}), 400

    if not task.result_path or not os.path.exists(task.result_path):
        return jsonify({"error": "result file not found"}), 404

    return send_file(
        task.result_path,
        as_attachment=True,
        download_name=f'{task.city}_city.json'
    )

@bp.route("/upload_building_files",methods=['POST'])
def upload_building_files():
    files = request.files.getlist('building_files[]')
    if not files:
        return jsonify({'error': 'file not found'})
    file_dir = DATA_DIR / 'actors'
    if not file_dir.exists():
        file_dir.mkdir()
    saved = []
    for file in files:
        file_path = file_dir / file.filename
        if file_path.exists():
            file_path.unlink()
        file.save(file_path)
        saved.append(file_path.name)
    return jsonify({'files': saved})
    
@bp.route('/get_citymask',methods=['POST'])
def get_citymask():
    data = request.get_json()
    city = data['city']
    session = SessionLocal()
    queryTask = session.query(StreatBlockTask).filter_by(city=city).first()
    if not queryTask:
        session.close()
        return jsonify({'error': f'{city} not found in street block results, Please create street block data first.'})
    
    subprocess.run([
        'python', '/home/pixel/geo_tools/city_process/create_city_mask.py', 
        city,
    ])
    city_mask_path = DATA_DIR / 'city'/ city / 'city.mask'
    if not city_mask_path.exists():
        return jsonify({'error': 'city mask not found'})
    return send_file(
        city_mask_path,
        as_attachment=True,
        download_name=f'{city}_city.mask'
    )

@bp.route('/get_city_terrain',methods=['POST'])
def get_city_terrain():
    data = request.get_json()
    city = data['city']
    session = SessionLocal()
    queryTask = session.query(StreatBlockTask).filter_by(city=city).first()
    if not queryTask:
        session.close()
        return jsonify({'error': f'{city} not found in street block results, Please create street block data first.'})
    
    subprocess.run([
        'python', '/home/pixel/geo_tools/city_process/smooth_terrain.py', 
        city,
    ])
    terrain_path = DATA_DIR / 'city'/ city / 'terrain.zip'
    if not terrain_path.exists():
        return jsonify({'error': 'terrain file not found'})
    return send_file(
        terrain_path,
        as_attachment=True,
        download_name=f'{city}_terrain.zip'
    )

@bp.route('/get_poi',methods=['GET'])
def get_poi():
    x = request.args.get('x')
    y = request.args.get('y')
    radius = request.args.get('radius')
    in_game_coord = request.args.get('in_game')
    ingame_bool = True if in_game_coord == 'true' else False
    radius = radius if radius else 1000
    process = subprocess.Popen([
        'python', '/home/pixel/geo_tools/overpass_query/query_poi.py', 
        '-r', radius,
        '-g', 'True' if ingame_bool else 'False',
        '--',
        x, y
    ],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE)
    stdout, stderr = process.communicate()
    if process.returncode != 0:
        return jsonify({'error': stderr.decode('utf-8')})
    out_str = stdout.decode('utf-8')
    if not out_str:
        return jsonify({'error': 'no result found'})
    content = re.sub(r'//.*', '', out_str)
    return jsonify(json.loads(content))

@bp.route('/help',methods=['GET'])
def helpPDF():
    return send_file(
        DATA_DIR / 'README.pdf',
        download_name='README.pdf'
    )

@bp.route('/config',methods=['GET'])
def config():
    return render_template('config.html')

@bp.route('/myip',methods=['GET'])
def myip():
    request_ip = request.remote_addr
    return jsonify({'ip': request_ip})