#!/bin/bash
set -e

echo "=== Setting up ELK Stack for Embedded UI Designer Observability ==="
echo "1. Checking Docker and Docker Compose..."
if ! command -v docker &> /dev/null; then
    echo "Docker not found, proceeding with local configuration templates."
fi

mkdir -p ./observability/elk
cat << 'EOF' > ./observability/elk/docker-compose.yml
version: '3.8'
services:
  elasticsearch:
    image: docker.elastic.co/elasticsearch/elasticsearch:8.9.0
    container_name: elasticsearch
    environment:
      - discovery.type=single-node
      - xpack.security.enabled=false
      - "ES_JAVA_OPTS=-Xms512m -Xmx512m"
    ports:
      - "9200:9200"
    networks:
      - elk-net

  logstash:
    image: docker.elastic.co/logstash/logstash:8.9.0
    container_name: logstash
    volumes:
      - ./logstash.conf:/usr/share/logstash/pipeline/logstash.conf
    ports:
      - "5044:5044"
      - "8080:8080/udp"
    networks:
      - elk-net
    depends_on:
      - elasticsearch

  kibana:
    image: docker.elastic.co/kibana/kibana:8.9.0
    container_name: kibana
    ports:
      - "5601:5601"
    environment:
      - ELASTICSEARCH_HOSTS=http://elasticsearch:9200
    networks:
      - elk-net
    depends_on:
      - elasticsearch

networks:
  elk-net:
    driver: bridge
EOF

cat << 'EOF' > ./observability/elk/logstash.conf
input {
  tcp {
    port => 5000
    codec => json
  }
}
filter {
  date {
    match => [ "timestamp", "ISO8601" ]
  }
}
output {
  elasticsearch {
    hosts => ["http://elasticsearch:9200"]
    index => "embedded-ui-audit-%{+YYYY.MM.dd}"
  }
}
EOF

chmod +x ./observability/elk/docker-compose.yml || true
echo "ELK stack configuration generated in ./observability/elk/"
echo "Ready for deployment via: docker-compose -f ./observability/elk/docker-compose.yml up -d"
