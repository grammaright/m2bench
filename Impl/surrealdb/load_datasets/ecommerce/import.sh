# create
sudo docker run --rm -i surrealdb/surrealdb:latest sql --endpoint http://192.168.0.4:8000 --namespace ns --database ecommerce -u root -p root < create_table.sql
sudo docker run --rm -i surrealdb/surrealdb:latest sql --endpoint http://192.168.0.4:8000 --namespace ns --database ecommerce -u root -p root < create_json.sql
sudo docker run --rm -i surrealdb/surrealdb:latest sql --endpoint http://192.168.0.4:8000 --namespace ns --database ecommerce -u root -p root < create_graph.sql
