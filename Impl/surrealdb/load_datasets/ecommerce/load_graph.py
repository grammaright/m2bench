import csv

from surrealdb import Surreal
from tqdm import tqdm

DATAPATH = '/tmp/m2bench/ecommerce'

with Surreal("ws://localhost:8000/rpc") as db:
    db.signin({"username": 'root', "password": 'root'})
    db.use("ns", "ecommerce")

    # import nodes
    print("importing person")
    with open(DATAPATH + '/property_graph/person_node.csv') as csvfile:
        lines = csv.reader(csvfile, delimiter='|')
        header = True
        for row in tqdm(lines):
            if header:
                header = False
                continue

            db.create(
                "person",
                {
                    'person_id': int(row[0]),
                    'gender': row[1],
                    'date_of_brith': row[2],
                    'firstname': row[3],
                    'lastname': row[4],
                    'nationality': row[5],
                    'email': row[6]
                },
            )


    print("importing hashtag")
    with open(DATAPATH + '/property_graph/hashtag_node.csv') as csvfile:
        lines = csv.reader(csvfile, delimiter=',')
        header = True
        for row in tqdm(lines):
            if header:
                header = False
                continue

            db.create(
                "hashtag",
                {
                    "tag_id": int(row[0]),
                    "content": row[1]
                },
            )

    # import edges
    print("importing follows")
    with open(DATAPATH + '/property_graph/person_follows_person.csv') as csvfile:
        lines = csv.reader(csvfile, delimiter='|')
        header = True
        for row in tqdm(lines):
            if header:
                header = False
                continue

            # find nodes first
            _from = db.query("SELECT id FROM person WHERE person_id = " + row[0])[0]['id'];
            _to = db.query("SELECT id FROM person WHERE person_id = " + row[1])[0]['id'];

            # insert edge; 
            # _from and _to are node perspective and in/out are edge perspective
            db.insert_relation("follows", {
                "in": _from,
                "out": _to,
                "created_time": row[2]
            })

    print("importing interested_in")
    with open(DATAPATH + '/property_graph/person_interestedIn_tag.csv') as csvfile:
        lines = csv.reader(csvfile, delimiter=',')
        header = True
        for row in tqdm(lines):
            if header:
                header = False
                continue

            # find nodes first
            _from = db.query("SELECT id FROM person WHERE person_id = " + row[0])[0]['id'];
            _to = db.query("SELECT id FROM hashtag WHERE tag_id = " + row[1])[0]['id'];

            # insert edge
            # _from and _to are node perspective and in/out are edge perspective
            db.insert_relation("interested_in", {
                "in": _from,
                "out": _to,
                "created_time": row[2]
            })


