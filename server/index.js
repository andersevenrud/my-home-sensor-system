/*
 * my-home-sensor-system
 * @author Anders Evenrud <andersevenrud@gmail.com>
 * @license CC BY
 */

const Serial = require('serialport')
const Delimiter = require('@serialport/parser-delimiter')
const client = require('prom-client')
const express = require('express')

const SERIAL = '/dev/ttyACM0'
const PORT = 3010
const MATCHER = /^grove_sensor/

const gauges = {}
const server = express()
const port = new Serial(SERIAL, { autoOpen: false, baudRate: 9600 })
const parser = port.pipe(new Delimiter({ delimiter: '\n' }))
const state = { open: false }

let timeout

const createTimeout = () => {
  clearTimeout(timeout)
  timeout = setTimeout(() => {
    process.exit(1)
  }, 1000 * 30)
}

const check = (req, res, next) => state.open
  ? next()
  : res.status(503).end()

const metrics = (req, res) => client.register.metrics()
  .then(str => res.end(str))
  .catch(error => res.status(500).json({ error }))

const parse = (data) =>  {
  const str = data.toString()
  if (str.match(MATCHER)) {
    try {
      const [k, v] = str.split(' ')
      gauges[k] = gauges[k] || new client.Gauge({ name: k, help: k })
      gauges[k].set(v.match(/^\d+$/) ? parseInt(v) : parseFloat(v))
    } catch (e) {
      console.warn(e)
    }
  }
}

port.on('open', () => (state.open = true))
port.on('open', () => console.log('Serial port', SERIAL, 'opened...'))
port.on('error', (err) => console.error('Serial port error', err))
port.on('error', () => process.exit(1))
port.on('close', () => process.exit(1))
parser.on('data', parse);
server.get('/metrics', check, metrics)
server.listen(PORT, () => console.info('Listening on', PORT))
port.open((error) => {
  if (error) {
    console.error(error)
    process.exit(1)
  }
})
