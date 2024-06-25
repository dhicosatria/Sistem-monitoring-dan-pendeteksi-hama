# Use the official Node-RED base image
FROM nodered/node-red:latest

# Expose the port
EXPOSE 1880

# Set the working directory
WORKDIR /usr/src/node-red

# Copy package.json to the WORKDIR
COPY package.json .

# Install any additional nodes
RUN npm install

# Copy user files
COPY . .

# Start Node-RED
CMD ["npm", "start", "--", "--userDir", "/data"]
