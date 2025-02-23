import csv

from surrealdb import Surreal
from tqdm import tqdm

DATAPATH = '/tmp/m2bench/ecommerce'

with Surreal("ws://localhost:8000/rpc") as db:
    db.signin({"username": 'root', "password": 'root'})
    db.use("ns", "ecommerce")

    print("importing brand")
    with open(DATAPATH + '/table/Brand.csv') as csvfile:
        lines = csv.reader(csvfile, delimiter=',')
        header = True
        for row in tqdm(lines):
            if header:
                header = False
                continue

            db.create(
                "brand",
                {
                    "brand_id": int(row[0]),
                    "name": row[1],
                    "country": row[2],
                    "industry": row[3],
                },
            )

    print("importing customer")
    with open(DATAPATH + '/table/Customer.csv') as csvfile:
        lines = csv.reader(csvfile, delimiter='|')
        header = True
        for row in tqdm(lines):
            if header:
                header = False
                continue

            db.create(
                "customer",
                {
                    "customer_id": row[0],
                    "person_id": int(row[1]),
                    "gender": row[2],
                    "date_of_birth": row[3],
                    "zipcode": row[4],
                    "city": row[5],
                    "county": row[6],
                    "state": row[7],
                },
            )


    print("importing product")
    with open(DATAPATH + '/table/Product.csv') as csvfile:
        lines = csv.reader(csvfile, delimiter=',')
        header = True
        for row in tqdm(lines):
            if header:
                header = False
                continue

            db.create(
                "product",
                {
                    "product_id": row[0],
                    "title": row[1],
                    "price": float(row[2]),
                    "brand_id": int(row[3]),
                },
            )

