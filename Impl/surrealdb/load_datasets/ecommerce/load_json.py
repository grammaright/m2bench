import json

from surrealdb import Surreal
from tqdm import tqdm

DATAPATH = '/tmp/m2bench/ecommerce'

with Surreal("ws://localhost:8000/rpc") as db:
    db.signin({"username": 'root', "password": 'root'})
    db.use("ns", "ecommerce")

    print("importing order")
    with open(DATAPATH + '/json/order.json') as jsonObj:
        for line in tqdm(jsonObj):
            obj = json.loads(line)
            db.create("order", obj)

    print("importing review")
    with open(DATAPATH + '/json/review.json') as jsonObj:
        for line in tqdm(jsonObj):
            obj = json.loads(line)
            db.create("review", obj)

