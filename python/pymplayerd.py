import socket
import string

class mplayerd:
	def __init__(self):
		self.sock = None
		# 0 stopped, 1 playing, 2 paused
		self.host = 'localhost'
		self.port = 7400
		self.eof = chr(160)

		self.error = None

	def connect(self, host = 'localhost', port = 7400):
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

		try:
			sock.connect( (host, port) )
		except:
			return False

		self.host = host
		self.port = port

		self.sock = sock
		self.read()
		
		return True

	def isConnected(self):
		if self.sock is None:
			return False
		return True

	def disconnect(self):
		if not self.sock: return False
		self.sock.close()
		self.sock = None
		return True


	def read(self):
		if not self.sock: return False

		buf = ''
		while True:
			tmp = self.sock.recv(4096)
			buf += tmp
			if (buf.find(self.eof) >= 0): break

		return buf

	def write(self, buf):
		self.sock.send(buf)

	def command(self, cmd):
		self.write(cmd + "\r\n")
		return self.read()


	def responseOK(self, buf):
		[resp] = buf.split("\r\n")[1:2]
		tmp = resp.split(" ") 

		code = int(tmp[0])
		response = tmp[1:]
		self.error = None
		self.response = response
		if code: return True
		self.error = response
		return False


	def status(self, instance = 1):
		buf = self.command("status")

		rv = {}

		for f in buf.split("\r\n"):
			
			i = f.find(":")

			if (i == -1): continue

			rv[ f[0:i] ] = f[i+1:].strip()

		return rv

	def cd(self, directory):
		buf = self.command('cd 1 "%s"' % (directory) )
		lines = buf.split("\r\n")

		if lines[1] == 'No such file or directory':
			return False

		return True
	def load(self, file, instance = 1):
		buf = self.command('load %d "%s"' % (instance, file))
		return self.responseOK(buf)


	def stop(self, instance = 1):
		buf = self.command("stop %d" % (instance))
		return self.responseOK(buf)

	def pause(self, instance = 1):
		buf = self.command("pause %d" % (instance))
		return self.responseOK(buf)
	
	def fullscreen(self, instance = 1):
		buf = self.command("fullscreen %d" % (instance))
		return self.responseOK(buf)

	def osd(self, level = 1, instance = 1):
		buf = self.command('osd %d %d' % (instance, level) )
		return self.responseOK(buf)	

	def volume(self, direction, instance = 1):
		buf = self.command('volume %d %s' % (instance, direction) )
		return self.responseOK(buf)


	def mute(self, instance = 1):
		buf = self.command('mute %d' % (instance) )
		return self.responseOK(buf)

	def version(self):
		buf = self.command('version')
		resp = buf.split("\r\n")[1]
		return resp


	def shutdown(self):
		buf = self.command('shutdown')
		return self.responseOK(buf)

	def pwd(self):
		buf = self.command('pwd')
		resp = buf.split("\r\n")[1:-1][0]
		return resp

	def ls(self, path = None):
		cmd = 'ls 1'
		if not path is None:
			cmd += ' "%s"' % (path)
		buf = self.command(cmd)
		lines = buf.split("\r\n")

		list = []

		for f in lines[1:-1]:
			i = f.find(" ")
			if f[-1] == '/':
				type = 'd'
				name = f[i:-1].strip()
			else:
				type = 'f'
				name = f[i:].strip()

			id = int(f[:i])

			# list.append( (id, type, name) )
			yield (id, type, name)
		

	def who(self):
		buf = self.command('who')
		lines = buf.split("\r\n")
		l = len(lines)
		who = lines[2:l - 2]
		resp = []
		for w in who:
			[ID,IP,CWD] = w.split("\t\t")
			resp.append( (ID, IP, CWD) ) 

		return len(resp), resp

	def kill(self, session):
		buf = self.command('kill 1 %d' % (session) )
		return "raw", buf

	def instances(self):
		buf = self.command('instances')
		lines = buf.split("\r\n")
		l = len(lines)
		instances = []
		for inst in lines[2:l - 1]:
			tmp = inst.split(" ")
			[id, status] = tmp[0:2]
			info = tmp[2:]
			instances.append( (id, status, info) )	
		return len(instances), instances

	def new(self):
		buf = self.command('new')
		if not self.responseOK(buf): return False

		[new] = buf.split("\r\n")[3]
		return new

	def id3(self, filename, instance = 1):
		buf = self.command('id3 %d "%s"' % (instance, filename))
		id3 = {}

		for line in buf.split("\r\n")[2:-1]:

			p = line.find(":")
			if p == -1:
				continue

			var = line[:p]
			val = line[p + 1:].strip()

			if val == '':
				val = 'none'

			id3[var] = val

		return id3
