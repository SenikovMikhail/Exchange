\#!/bin/bash

set -e

DB_NAME="task"
DB_USER="adm"
DB_USER_PASS="adm123"


sudo su postgres <<EOF

psql -c "CREATE USER $DB_USER WITH PASSWORD '$DB_USER_PASS';"
psql -c "grant all privileges on database $DB_NAME to $DB_USER;"
createdb --owner=$DB_USER  $DB_NAME;
echo "Postgres User '$DB_USER' and database '$DB_NAME' created."
psql "postgresql://$DB_USER:$DB_USER_PASS@localhost:5432/$DB_NAME" -q -c "CREATE TABLE users(user_id int primary key, user_name varchar(64), password_hash varchar(65), create_time varchar(25), balance_usd float(53), balance_rub float(53));"
psql "postgresql://$DB_USER:$DB_USER_PASS@localhost:5432/$DB_NAME" -q -c "CREATE TABLE orders(order_id int primary key, user_id int, pair varchar(8), order_type varchar(6), volum float(53), price float(53), open_time varchar(25), close_time varchar(25));"
EOF
